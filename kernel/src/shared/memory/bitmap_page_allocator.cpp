/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "shared/memory/bitmap_page_allocator.hpp"
#include "shared/logger/logger.hpp"
#include "shared/memory/constants.hpp"
#include "shared/system/mutex.hpp"
#include "shared/panic.hpp"

void _bitmapPageAllocatorFastBufferInitialize(g_bitmap_page_allocator* allocator);
bool _bitmapPageAllocatorFastBufferFree(g_bitmap_page_allocator* allocator, g_physical_address address);
g_physical_address _bitmapPageAllocatorFastBufferAllocate(g_bitmap_page_allocator* allocator);
g_physical_address _bitmapPageAllocatorEarlyAllocate(limine_memmap_response* memoryMap, int pages);
g_bitmap_header* _bitmapPageAllocatorInitializeBitmap(g_physical_address addr);


void bitmapPageAllocatorInitialize(g_bitmap_page_allocator* allocator, limine_memmap_response* memoryMap)
{
	mutexInitializeGlobal(&allocator->lock);
	allocator->freePageCount = 0;

	// Allocate top-level index page that keeps pointers to bitmaps
	g_physical_address indexPagePhys = _bitmapPageAllocatorEarlyAllocate(memoryMap, 1);
	allocator->indexPage = (g_bitmap_index_page_header*) G_MEM_PHYS_TO_VIRT(indexPagePhys);
	for(size_t i = 0; i < G_BITMAP_INDEX_MAX_ENTRIES; i++)
		allocator->indexPage->entries[i] = nullptr;

	int indexEntry = 0;
	for(int i = 0; i < memoryMap->entry_count; i++)
	{
		auto entry = memoryMap->entries[i];
		if(entry->type != LIMINE_MEMMAP_USABLE)
			continue;

		g_physical_address currentPage = entry->base;

		// At the start of every free area, create a bitmap
		auto bitmap = _bitmapPageAllocatorInitializeBitmap(currentPage);
		allocator->indexPage->entries[indexEntry++] = bitmap;
		currentPage += G_PAGE_SIZE;

		while(currentPage < entry->base + entry->length)
		{
			size_t entryIndex = (currentPage - bitmap->base) / G_PAGE_SIZE;
			size_t bitmapIndex = entryIndex / G_BITMAP_BITS_PER_ENTRY;
			size_t bitmapBit = entryIndex % G_BITMAP_BITS_PER_ENTRY;
			bitmap->entries[bitmapIndex] |= (1 << bitmapBit);
			allocator->freePageCount++;

			// Go to next page
			currentPage += G_PAGE_SIZE;
			bitmap->end = currentPage;

			// Check if we have reached the end of this bitmap
			bool bitmapFull = entryIndex == G_BITMAP_TOTAL_BITS - 1;
			if(bitmapFull)
			{
				bool remainingSpaceSufficient = currentPage + G_PAGE_SIZE < entry->base + entry->length;
				if(!remainingSpaceSufficient)
					break;

				// Create the next bitmap at the next free page
				bitmap = _bitmapPageAllocatorInitializeBitmap(currentPage);
				allocator->indexPage->entries[indexEntry++] = bitmap;
				currentPage += G_PAGE_SIZE;

				// Currently only one index page is supported
				if(indexEntry >= G_BITMAP_INDEX_MAX_ENTRIES)
					goto stopFilling;
			}
		}
	}

stopFilling:
	_bitmapPageAllocatorFastBufferInitialize(allocator);
}

g_bitmap_header* _bitmapPageAllocatorInitializeBitmap(g_physical_address addrPhys)
{
	auto bitmap = (g_bitmap_header*) G_MEM_PHYS_TO_VIRT(addrPhys);

	bitmap->base = addrPhys + G_PAGE_SIZE;
	bitmap->end = addrPhys + G_PAGE_SIZE;
	mutexInitializeGlobal(&bitmap->lock);

	for(size_t entry = 0; entry < G_BITMAP_MAX_ENTRIES; entry++)
		bitmap->entries[entry] = 0;

	return bitmap;
}


/**
 * Early allocation function that modifies the memory map to simply cut off the
 * requested amount of pages from one of the usable areas. Does not use the
 * lower memory area.
 */
g_physical_address _bitmapPageAllocatorEarlyAllocate(limine_memmap_response* memoryMap, int pages)
{
	size_t allocatedSize = pages * G_PAGE_SIZE;

	for(int i = 0; i < memoryMap->entry_count; i++)
	{
		auto entry = memoryMap->entries[i];
		if(entry->type != LIMINE_MEMMAP_USABLE)
			continue;

		if(entry->base >= G_MEM_LOWER_END && entry->length > allocatedSize)
		{
			g_physical_address address = entry->base + entry->length - allocatedSize;
			entry->length -= allocatedSize;
			return address;
		}
	}

	panic("%! failed to allocate %i physical pages", "bitmap", pages);
}


void bitmapPageAllocatorMarkFree(g_bitmap_page_allocator* allocator, g_physical_address address)
{
	if(G_PAGE_ALIGN_DOWN(address) != address)
		panic("%! attempted to free unaligned physical address %x", "bitmap", address);

	if(_bitmapPageAllocatorFastBufferFree(allocator, address))
		return;

	bool success = false;

	for(size_t i = 0; i < G_BITMAP_INDEX_MAX_ENTRIES; i++)
	{
		g_bitmap_header* bitmap = allocator->indexPage->entries[i];
		if(!bitmap)
			break;

		// TODO: Try acquire would be cool here
		mutexAcquire(&bitmap->lock);
		if(address >= bitmap->base && address < bitmap->end)
		{
			size_t totalBitIndex = (address - bitmap->base) / G_PAGE_SIZE;
			size_t bitmapIndex = totalBitIndex / G_BITMAP_BITS_PER_ENTRY;
			size_t bitmapBit = totalBitIndex % G_BITMAP_BITS_PER_ENTRY;
			bitmap->entries[bitmapIndex] &= ~(1ULL << bitmapBit);

			mutexAcquire(&allocator->lock);
			allocator->freePageCount++;
			mutexRelease(&allocator->lock);

			success = true;
		}
		mutexRelease(&bitmap->lock);

		if(success)
			break;
	}

	if(!success)
		logWarn("%! failed to free physical address %x", "bitmap", address);
}

g_physical_address bitmapPageAllocatorAllocate(g_bitmap_page_allocator* allocator)
{
	g_physical_address addressFromFB = _bitmapPageAllocatorFastBufferAllocate(allocator);
	if(addressFromFB)
		return addressFromFB;

	for(size_t bitmapIndex = 0; bitmapIndex < G_BITMAP_INDEX_MAX_ENTRIES; bitmapIndex++)
	{
		auto bitmap = allocator->indexPage->entries[bitmapIndex];
		if(!bitmap)
			break;

		mutexAcquire(&bitmap->lock);
		g_physical_address result = 0;
		for(size_t entryIndex = 0; entryIndex < G_BITMAP_MAX_ENTRIES; entryIndex++)
		{
			auto entry = bitmap->entries[entryIndex];
			if(entry == 0)
				continue;

			for(int bit = 0; bit < sizeof(entry) * 8; bit++)
			{
				if(entry & (1ULL << bit))
				{
					bitmap->entries[entryIndex] &= ~(1ULL << bit);
					size_t totalBitIndex = entryIndex * G_BITMAP_BITS_PER_ENTRY + bit;

					mutexAcquire(&allocator->lock);
					allocator->freePageCount--;
					mutexRelease(&allocator->lock);

					result = bitmap->base + totalBitIndex * G_PAGE_SIZE;
					goto allocated;
				}
			}
		}
	allocated:
		mutexRelease(&bitmap->lock);
		if(result > 0)
			return result;
	}

	panic("%! failed to allocate physical memory", "bitmap");
}

/**
 * Initializes the "fast buffer" which is used to keep a stack of free pages for faster allocation.
 *
 * @param allocator
 */
void _bitmapPageAllocatorFastBufferInitialize(g_bitmap_page_allocator* allocator)
{
	mutexInitializeGlobal(&allocator->fastBuffer.lock);
	for(g_physical_address& cell: allocator->fastBuffer.buffer)
	{
		cell = 0;
	}
	allocator->fastBuffer.size = 0;
}

bool _bitmapPageAllocatorFastBufferFree(g_bitmap_page_allocator* allocator, g_physical_address address)
{
	bool freed = false;

	mutexAcquire(&allocator->fastBuffer.lock);
	if(allocator->fastBuffer.size < G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE)
	{
		allocator->fastBuffer.buffer[allocator->fastBuffer.size++] = address;
		freed = true;
	}
	mutexRelease(&allocator->fastBuffer.lock);

	mutexAcquire(&allocator->lock);
	allocator->freePageCount++;
	mutexRelease(&allocator->lock);

	return freed;
}

g_physical_address _bitmapPageAllocatorFastBufferAllocate(g_bitmap_page_allocator* allocator)
{
	g_physical_address address = 0;

	mutexAcquire(&allocator->fastBuffer.lock);
	if(allocator->fastBuffer.size > 0)
	{
		address = allocator->fastBuffer.buffer[--allocator->fastBuffer.size];
	}
	mutexRelease(&allocator->fastBuffer.lock);

	mutexAcquire(&allocator->lock);
	allocator->freePageCount--;
	mutexRelease(&allocator->lock);

	return address;
}
