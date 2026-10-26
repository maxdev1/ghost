/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2024, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "kernel/memory/allocator.hpp"
#include "kernel/utils/math.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/panic.hpp"
#include "kernel/utils/string.hpp"

/**
 * This memory allocator uses a combination of buckets and linked-lists. For all allocations that are
 * smaller or equal to G_ALLOCATOR_MAX_FOR_BUCKETS, a bucket section is used with a bitmap that keeps
 * track of available memory. For larger allocations, a section is assigned directly.
 */

void _memoryAllocatorMerge(g_allocator* allocator);
g_allocator_section_header* _memoryAllocatorAllocateSection(g_allocator* allocator, g_size totalSize,
                                                            g_allocator_section_type type);
g_size _memoryAllocatorFreeSection(g_allocator* allocator, g_allocator_section_header* section);
g_size _memoryAllocatorFreeInBucket(g_allocator* allocator, g_allocator_section_bucket* bucket, void* mem);
void* _memoryAllocatorAllocateInBucket(g_allocator* allocator, g_size size);
void* _memoryAllocatorAllocateInSpecificBucket(g_allocator_section_bucket* bucket);

#define _G_ALLOCATOR_BUCKET_BITMAP(bucket) ((uint8_t*) ((g_address) bucket + sizeof(g_allocator_section_bucket)))
#define _G_ALLOCATOR_BUCKET_CONTENT_OFFSET(bitmapSize) G_ALLOCATOR_ALIGN_UP(sizeof(g_allocator_section_bucket) + (bitmapSize))
#define _G_ALLOCATOR_BUCKET_CONTENT(bucket) ((uint8_t*) ((g_address) bucket + _G_ALLOCATOR_BUCKET_CONTENT_OFFSET(bucket->bitmapSize)))

void memoryAllocatorInitialize(g_allocator* allocator, g_allocator_type type, g_virtual_address start,
                               g_virtual_address end)
{
	mutexInitializeGlobal(&allocator->lock, __func__);

	allocator->type = type;

	auto firstSection = (g_allocator_section_header*) start;
	firstSection->type = G_ALLOCATOR_SECTION_TYPE_FREE;
	firstSection->totalSize = end - start;
	firstSection->next = nullptr;
	allocator->sections = firstSection;
}

void* memoryAllocatorAllocate(g_allocator* allocator, g_size size, g_size* allocatedSize)
{
	if(allocatedSize)
		*allocatedSize = 0;
	mutexAcquire(&allocator->lock);

	if(!allocator->sections)
		panic("%! tried to use allocate on uninitialized chunk allocator at %x", "alloc", allocator);

	size = size < G_ALLOCATOR_ALIGNMENT ? G_ALLOCATOR_ALIGNMENT : mathCeilToPowerOf2(size);
	if(!size || size > (g_size)-1 - sizeof(g_allocator_section_header))
	{
		mutexRelease(&allocator->lock);
		return nullptr;
	}

	void* mem = nullptr;
	if(size > G_ALLOCATOR_MAX_FOR_BUCKETS)
	{
		auto section = _memoryAllocatorAllocateSection(allocator, sizeof(g_allocator_section_header) + size,
		                                               G_ALLOCATOR_SECTION_TYPE_CHUNK);
		if(section)
		{
			mem = (void*) ((g_address) section + sizeof(g_allocator_section_header));
			size = section->totalSize - sizeof(g_allocator_section_header);
		}
	}
	else
		mem = _memoryAllocatorAllocateInBucket(allocator, size);

	if(mem && allocatedSize)
		*allocatedSize = size;
	mutexRelease(&allocator->lock);
	return mem;
}

g_size memoryAllocatorFree(g_allocator* allocator, void* mem)
{
	if(!mem)
		return 0;

	mutexAcquire(&allocator->lock);

	const auto address = (g_address) mem;
	auto section = allocator->sections;
	while(section)
	{
		const auto base = (g_address) section;
		if(address >= base && address - base < section->totalSize)
			break;
		section = section->next;
	}

	if(!section)
	{
		logWarn("%! attempted to free %x not managed by kernel allocator (type %i)", "alloc", mem, allocator->type);
		mutexRelease(&allocator->lock);
		return 0;
	}

	g_size size = 0;
	if(section->type == G_ALLOCATOR_SECTION_TYPE_CHUNK)
	{
		if(address == (g_address) section + sizeof(g_allocator_section_header))
			size = _memoryAllocatorFreeSection(allocator, section);
	}
	else if(section->type == G_ALLOCATOR_SECTION_TYPE_BUCKET)
		size = _memoryAllocatorFreeInBucket(allocator, (g_allocator_section_bucket*) section, mem);
	else if(section->type != G_ALLOCATOR_SECTION_TYPE_FREE)
		panic("%! attempt to free kernel memory in a section of type %i in allocator %i", "alloc", section->type,
		      allocator->type);

	if(!size)
		logWarn("%! rejected invalid or already-free kernel memory %x in allocator %i", "alloc", mem, allocator->type);
	mutexRelease(&allocator->lock);
	return size;
}

void memoryAllocatorExpand(g_allocator* allocator, g_size size)
{
	mutexAcquire(&allocator->lock);

	g_allocator_section_header* last = allocator->sections;
	while(last->next)
		last = last->next;

	auto next = (g_allocator_section_header*) ((g_address) last + last->totalSize);
	next->type = G_ALLOCATOR_SECTION_TYPE_FREE;
	next->totalSize = size;
	next->next = nullptr;
	last->next = next;

	_memoryAllocatorMerge(allocator);

	mutexRelease(&allocator->lock);
}

g_allocator_section_header* _memoryAllocatorAllocateSection(g_allocator* allocator, g_size totalSize,
                                                            g_allocator_section_type type)
{
	g_allocator_section_header* current = allocator->sections;
	do
	{
		if(current->type == G_ALLOCATOR_SECTION_TYPE_FREE && current->totalSize >= totalSize)
			break;

		current = current->next;
	} while(current);

	if(!current)
		return nullptr;

	g_size remainder = current->totalSize - totalSize;
	if(remainder > sizeof(g_allocator_section_header) + G_ALLOCATOR_MAX_FOR_BUCKETS)
	{
		auto next = (g_allocator_section_header*) ((g_address) current + totalSize);
		current->totalSize = totalSize;
		next->next = current->next;
		current->next = next;
		next->type = G_ALLOCATOR_SECTION_TYPE_FREE;
		next->totalSize = remainder;
	}

	current->type = type;
	return current;
}

g_size _memoryAllocatorFreeSection(g_allocator* allocator, g_allocator_section_header* section)
{
	g_size size = section->totalSize - sizeof(g_allocator_section_header);
	section->type = G_ALLOCATOR_SECTION_TYPE_FREE;
	_memoryAllocatorMerge(allocator);
	return size;
}

g_allocator_section_bucket* _memoryAllocatorAllocateNewBucket(g_allocator* allocator, g_size size)
{
	uint16_t entryCount;
	if(size <= 256)
		entryCount = 8192 / size;
	else
		entryCount = 16;

	auto bitmapSize = (entryCount / 8);
	auto sectionTotalSize = _G_ALLOCATOR_BUCKET_CONTENT_OFFSET(bitmapSize) + entryCount * size;

	auto bucket = (g_allocator_section_bucket*) _memoryAllocatorAllocateSection(
			allocator, sectionTotalSize, G_ALLOCATOR_SECTION_TYPE_BUCKET);
	if(!bucket)
		return nullptr;

	bucket->entrySize = size;
	bucket->bitmapSize = bitmapSize;

	uint8_t* bucketBitmap = _G_ALLOCATOR_BUCKET_BITMAP(bucket);
	for(uint16_t i = 0; i < bucket->bitmapSize; i++)
		bucketBitmap[i] = 0;

	return bucket;
}

void* _memoryAllocatorAllocateInSpecificBucket(g_allocator_section_bucket* bucket)
{
	uint8_t* bucketBitmap = _G_ALLOCATOR_BUCKET_BITMAP(bucket);
	int byteIndex = -1;
	for(uint16_t i = 0; i < bucket->bitmapSize; i++)
	{
		if(bucketBitmap[i] < 0xFF)
		{
			byteIndex = i;
			break;
		}
	}

	if(byteIndex == -1)
		return nullptr;

	uint8_t byte = bucketBitmap[byteIndex];
	uint8_t bitIndex;
	for(uint8_t i = 0; i < 8; i++)
	{
		if((byte & (1 << i)) == 0)
		{
			bitIndex = i;
			break;
		}
	}

	bucketBitmap[byteIndex] |= 1 << bitIndex;

	g_size offset = byteIndex * (bucket->entrySize * 8) + bitIndex * bucket->entrySize;
	return &_G_ALLOCATOR_BUCKET_CONTENT(bucket)[offset];
}

void* _memoryAllocatorAllocateInBucket(g_allocator* allocator, g_size size)
{
	auto section = allocator->sections;
	do
	{
		if(section->type == G_ALLOCATOR_SECTION_TYPE_BUCKET)
		{
			auto bucket = (g_allocator_section_bucket*) section;
			if(bucket->entrySize == size)
			{
				void* result = _memoryAllocatorAllocateInSpecificBucket(bucket);
				if(result)
					return result;
			}
		}

		section = section->next;
	} while(section);

	g_allocator_section_bucket* bucket = _memoryAllocatorAllocateNewBucket(allocator, size);
	if(!bucket)
		return nullptr;

	return _memoryAllocatorAllocateInSpecificBucket(bucket);
}

g_size _memoryAllocatorFreeInBucket(g_allocator* allocator, g_allocator_section_bucket* bucket, void* mem)
{
	const auto contentAddress = (g_address) _G_ALLOCATOR_BUCKET_CONTENT(bucket);
	const auto address = (g_address) mem;
	if(!bucket->entrySize || address < contentAddress)
		return 0;

	const auto offset = address - contentAddress;
	const auto entryIndex = offset / bucket->entrySize;
	if(offset % bucket->entrySize || entryIndex >= (g_size) bucket->bitmapSize * 8)
		return 0;

	uint8_t* bitmap = _G_ALLOCATOR_BUCKET_BITMAP(bucket);
	const uint8_t mask = 1U << (entryIndex % 8);
	if(!(bitmap[entryIndex / 8] & mask))
		return 0;
	bitmap[entryIndex / 8] &= ~mask;

	const g_size releasedSize = bucket->entrySize;
	for(uint16_t i = 0; i < bucket->bitmapSize; ++i)
		if(bitmap[i])
			return releasedSize;

	_memoryAllocatorFreeSection(allocator, &bucket->header);
	return releasedSize;
}

void _memoryAllocatorMerge(g_allocator* allocator)
{
	auto section = allocator->sections;
	while(section && section->next)
	{
		if(section->type == G_ALLOCATOR_SECTION_TYPE_FREE && section->next->type == G_ALLOCATOR_SECTION_TYPE_FREE)
		{
			section->totalSize += section->next->totalSize;
			section->next = section->next->next;
		}
		else
		{
			section = section->next;
		}
	}
}
