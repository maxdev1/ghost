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
#include "shared/logger/logger.hpp"
#include "shared/panic.hpp"
#include "shared/utils/string.hpp"

/**
 * This memory allocator uses a combination of buckets and linked-lists. For all allocations that are
 * smaller or equal to G_ALLOCATOR_MAX_FOR_BUCKETS, a bucket section is used with a bitmap that keeps
 * track of available memory. For larger allocations, a section is assigned directly.
 */

void _memoryAllocatorMerge(g_allocator* allocator);
g_allocator_section_header* _memoryAllocatorAllocateSection(g_allocator* allocator, g_size totalSize, g_allocator_section_type type);
g_size _memoryAllocatorFreeSection(g_allocator* allocator, g_allocator_section_header* section);
g_size _memoryAllocatorFreeInBucket(g_allocator* allocator, g_allocator_section_bucket* bucket, void* mem);
void* _memoryAllocatorAllocateInBucket(g_allocator* allocator, g_size size);
void* _memoryAllocatorAllocateInSpecificBucket(g_allocator* allocator, g_allocator_section_bucket* bucket);

#define _G_ALLOCATOR_BUCKET_BITMAP(bucket) ((uint8_t*) ((g_address) bucket + sizeof(g_allocator_section_bucket)))
#define _G_ALLOCATOR_BUCKET_CONTENT(bucket) ((uint8_t*) ((g_address) bucket + sizeof(g_allocator_section_bucket) + bucket->bitmapSize))

void memoryAllocatorInitialize(g_allocator* allocator, g_allocator_type type, g_virtual_address start, g_virtual_address end)
{
	mutexInitialize(&allocator->lock, true);

	allocator->type = type;

	g_allocator_section_header* firstSection = (g_allocator_section_header*) start;
	firstSection->type = G_ALLOCATOR_SECTION_TYPE_FREE;
	firstSection->totalSize = end - start;
	firstSection->next = nullptr;
	allocator->sections = (g_allocator_section_header*) firstSection;
}

void* memoryAllocatorAllocate(g_allocator* allocator, g_size size)
{
	mutexAcquire(&allocator->lock);

	if(!allocator->sections)
		panic("%! tried to use allocate on uninitialized chunk allocator at %x", "alloc", allocator);

	size = size < 8 ? 8 : mathCeilToPowerOf2(size);

	void* mem = nullptr;
	if(size > G_ALLOCATOR_MAX_FOR_BUCKETS)
	{
		auto section = _memoryAllocatorAllocateSection(allocator, sizeof(g_allocator_section_header) + size, G_ALLOCATOR_SECTION_TYPE_CHUNK);
		if(section)
			mem = (void*) ((g_address) section + sizeof(g_allocator_section_header));
	}
	else
		mem = _memoryAllocatorAllocateInBucket(allocator, size);

	mutexRelease(&allocator->lock);
	return (void*) mem;
}

g_size memoryAllocatorFree(g_allocator* allocator, void* mem)
{
	mutexAcquire(&allocator->lock);

	g_allocator_section_header* current = allocator->sections;
	do
	{
		if((g_address) mem > (g_address) current && (g_address) mem < (g_address) current + current->totalSize)
			break;
		current = current->next;
	} while(current);

	if(!current)
	{
		logInfo("%! attempted to free %x not managed by kernel allocator (type %i)", "critical", mem, allocator->type);
		mutexRelease(&allocator->lock);
		return 0;
	}

	g_size size;
	if(current->type == G_ALLOCATOR_SECTION_TYPE_CHUNK)
		size = _memoryAllocatorFreeSection(allocator, current);
	else if(current->type == G_ALLOCATOR_SECTION_TYPE_BUCKET)
		size = _memoryAllocatorFreeInBucket(allocator, (g_allocator_section_bucket*) current, mem);
	else
		panic("%! attempt to free kernel memory in a section of type %i in allocator %i", "alloc", current->type, allocator->type);

	mutexRelease(&allocator->lock);
	return size;
}

void memoryAllocatorExpand(g_allocator* allocator, g_size size)
{
	mutexAcquire(&allocator->lock);

	g_allocator_section_header* last = allocator->sections;
	while(last->next)
		last = last->next;

	g_allocator_section_header* next = (g_allocator_section_header*) ((g_address) last + last->totalSize);
	next->type = G_ALLOCATOR_SECTION_TYPE_FREE;
	next->totalSize = size;
	next->next = nullptr;
	last->next = next;

	_memoryAllocatorMerge(allocator);

	mutexRelease(&allocator->lock);
}

g_allocator_section_header* _memoryAllocatorAllocateSection(g_allocator* allocator, g_size totalSize, g_allocator_section_type type)
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
		g_allocator_section_header* next = (g_allocator_section_header*) ((g_address) current + totalSize);
		current->totalSize = totalSize;
		current->next = next;
		next->type = G_ALLOCATOR_SECTION_TYPE_FREE;
		next->totalSize = remainder;
		next->next = nullptr;
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
	auto sectionTotalSize = sizeof(g_allocator_section_bucket) + bitmapSize + entryCount * size;

	g_allocator_section_bucket* bucket = (g_allocator_section_bucket*) _memoryAllocatorAllocateSection(allocator, sectionTotalSize, G_ALLOCATOR_SECTION_TYPE_BUCKET);
	if(!bucket)
		return nullptr;

	bucket->entrySize = size;
	bucket->bitmapSize = bitmapSize;

	uint8_t* bucketBitmap = _G_ALLOCATOR_BUCKET_BITMAP(bucket);
	for(uint16_t i = 0; i < bucket->bitmapSize; i++)
		bucketBitmap[i] = 0;

	return bucket;
}

void* _memoryAllocatorAllocateInSpecificBucket(g_allocator* allocator, g_allocator_section_bucket* bucket)
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
		if((byte & (1 << i)) == 0)
			bitIndex = i;

	bucketBitmap[byteIndex] |= 1 << bitIndex;

	g_size offset = byteIndex * (bucket->entrySize * 8) + bitIndex * bucket->entrySize;
	return &_G_ALLOCATOR_BUCKET_CONTENT(bucket)[offset];
}

void* _memoryAllocatorAllocateInBucket(g_allocator* allocator, g_size size)
{
	g_allocator_section_header* current = allocator->sections;
	do
	{
		if(current->type == G_ALLOCATOR_SECTION_TYPE_BUCKET)
		{
			g_allocator_section_bucket* bucket = (g_allocator_section_bucket*) current;
			if(bucket->entrySize == size)
			{
				void* result = _memoryAllocatorAllocateInSpecificBucket(allocator, bucket);
				if(result)
					return result;
			}
		}

		current = current->next;
	} while(current);

	g_allocator_section_bucket* bucket = _memoryAllocatorAllocateNewBucket(allocator, size);
	return _memoryAllocatorAllocateInSpecificBucket(allocator, bucket);
}

g_size _memoryAllocatorFreeInBucket(g_allocator* allocator, g_allocator_section_bucket* bucket, void* mem)
{
	g_address contentAddress = (g_address) _G_ALLOCATOR_BUCKET_CONTENT(bucket);
	auto entryIndex = ((g_address) mem - contentAddress) / bucket->entrySize;

	uint8_t* bitmap = _G_ALLOCATOR_BUCKET_BITMAP(bucket);
	bitmap[entryIndex / 8] &= ~(1 << (entryIndex % 8));
	return bucket->entrySize;
}

void _memoryAllocatorMerge(g_allocator* allocator)
{
	g_allocator_section_header* current = (g_allocator_section_header*) allocator->sections;
	while(current && current->next)
	{
		if(current->type == G_ALLOCATOR_SECTION_TYPE_FREE && current->next->type == G_ALLOCATOR_SECTION_TYPE_FREE)
		{
			current->totalSize += current->next->totalSize;
			current->next = current->next->next;
		}
		else
		{
			current = current->next;
		}
	}
}