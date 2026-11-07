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

#include "kernel/memory/bitmap_page_allocator.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/panic.hpp"
#include "kernel/system/mutex.hpp"

void _bitmapPageAllocatorFastBufferInitialize(g_bitmap_page_allocator* allocator);
g_physical_address _bitmapPageAllocatorEarlyAllocate(limine_memmap_response* memoryMap, int pages);
g_bitmap_header* _bitmapPageAllocatorInitializeBitmap(g_physical_address addr);


void bitmapPageAllocatorInitialize(g_bitmap_page_allocator* allocator, limine_memmap_response* memoryMap)
{
	mutexInitializeGlobal(&allocator->lock);
	allocator->freePageCount = 0;
	allocator->firstBitmap = 0;

	// Allocate top-level index page that keeps pointers to bitmaps
	g_physical_address indexPagePhys = _bitmapPageAllocatorEarlyAllocate(memoryMap, 1);
	allocator->indexPage = (g_bitmap_index_page_header*) G_MEM_PHYS_TO_VIRT(indexPagePhys);
	allocator->indexPage->next = nullptr;
	for(size_t i = 0; i < G_BITMAP_INDEX_MAX_ENTRIES; i++)
	{
		allocator->indexPage->entries[i] = nullptr;
		allocator->firstWord[i] = 0;
	}

	int indexEntry = 0;
	for(int i = 0; i < memoryMap->entry_count; i++)
	{
		auto entry = memoryMap->entries[i];
		if(entry->type != LIMINE_MEMMAP_USABLE || entry->length < G_PAGE_SIZE)
			continue;

		g_physical_address currentPage = entry->base;
		// Firmware may report page zero and the AP trampoline area as usable.
		// Keep allocator metadata and frames above the reserved low megabyte.
		if(currentPage < G_MEM_LOWER_END)
			currentPage = G_MEM_LOWER_END;
		if(currentPage >= entry->base + entry->length)
			continue;

		// Currently only one index page is supported. Check before every insertion,
		// including the first bitmap of each separate firmware memory-map region.
		if(indexEntry >= G_BITMAP_INDEX_MAX_ENTRIES)
			goto stopFilling;

		// At the start of every free area, create a bitmap
		auto bitmap = _bitmapPageAllocatorInitializeBitmap(currentPage);
		allocator->indexPage->entries[indexEntry++] = bitmap;
		currentPage += G_PAGE_SIZE;

		while(currentPage < entry->base + entry->length)
		{
			size_t entryIndex = (currentPage - bitmap->base) / G_PAGE_SIZE;
			size_t bitmapIndex = entryIndex / G_BITMAP_BITS_PER_ENTRY;
			size_t bitmapBit = entryIndex % G_BITMAP_BITS_PER_ENTRY;
			bitmap->entries[bitmapIndex] |= (1ULL << bitmapBit);
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

				if(indexEntry >= G_BITMAP_INDEX_MAX_ENTRIES)
					goto stopFilling;

				// Create the next bitmap at the next free page
				bitmap = _bitmapPageAllocatorInitializeBitmap(currentPage);
				allocator->indexPage->entries[indexEntry++] = bitmap;
				currentPage += G_PAGE_SIZE;
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

		auto usableStart = entry->base < G_MEM_LOWER_END ? G_MEM_LOWER_END : entry->base;
		auto end = entry->base + entry->length;
		if(end > usableStart && end - usableStart > allocatedSize)
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

	for(size_t i = 0; i < G_BITMAP_INDEX_MAX_ENTRIES; ++i)
	{
		auto bitmap = allocator->indexPage->entries[i];
		if(!bitmap)
			break;
		if(address < bitmap->base || address >= bitmap->end)
			continue;

		const size_t bitIndex = (address - bitmap->base) / G_PAGE_SIZE;
		const size_t entryIndex = bitIndex / G_BITMAP_BITS_PER_ENTRY;
		const uint64_t mask = 1ULL << (bitIndex % G_BITMAP_BITS_PER_ENTRY);

		mutexAcquire(&bitmap->lock);
		if(bitmap->entries[entryIndex] & mask)
		{
			mutexRelease(&bitmap->lock);
			logWarn("%! attempted to free an available physical page %x", "bitmap", address);
			return;
		}

		// Both free and allocation acquire nested locks in bitmap, fast-buffer, budget order.
		mutexAcquire(&allocator->fastBuffer.lock);
		for(int cell = 0; cell < allocator->fastBuffer.size; ++cell)
		{
			if(allocator->fastBuffer.buffer[cell] == address)
			{
				mutexRelease(&allocator->fastBuffer.lock);
				mutexRelease(&bitmap->lock);
				logWarn("%! attempted to free a cached physical page %x", "bitmap", address);
				return;
			}
		}

		mutexAcquire(&allocator->lock);
		if(allocator->fastBuffer.size < G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE)
			allocator->fastBuffer.buffer[allocator->fastBuffer.size++] = address;
		else
		{
			bitmap->entries[entryIndex] |= mask;
			if(entryIndex < allocator->firstWord[i])
				allocator->firstWord[i] = entryIndex;
			if(i < allocator->firstBitmap)
				allocator->firstBitmap = i;
		}

		++allocator->freePageCount;
		mutexRelease(&allocator->lock);

		mutexRelease(&allocator->fastBuffer.lock);
		mutexRelease(&bitmap->lock);
		return;
	}

	logWarn("%! failed to free unmanaged physical address %x", "bitmap", address);
}

g_physical_address bitmapPageAllocatorTryAllocate(g_bitmap_page_allocator* allocator, uint32_t reservePages)
{
	mutexAcquire(&allocator->fastBuffer.lock);
	mutexAcquire(&allocator->lock);
	if(allocator->freePageCount <= reservePages)
	{
		mutexRelease(&allocator->lock);
		mutexRelease(&allocator->fastBuffer.lock);
		return 0;
	}
	--allocator->freePageCount;

	for(;;)
	{
		// Check both sources together: a concurrent free may have cached a frame
		// or rewound firstBitmap while we waited for a depleted bitmap's lock.
		if(allocator->fastBuffer.size > 0)
		{
			const auto address = allocator->fastBuffer.buffer[--allocator->fastBuffer.size];
			mutexRelease(&allocator->lock);
			mutexRelease(&allocator->fastBuffer.lock);
			return address;
		}
		const size_t bitmapIndex = allocator->firstBitmap;
		auto bitmap = bitmapIndex < G_BITMAP_INDEX_MAX_ENTRIES ? allocator->indexPage->entries[bitmapIndex] : nullptr;
		if(!bitmap)
			++allocator->freePageCount;
		mutexRelease(&allocator->lock);
		mutexRelease(&allocator->fastBuffer.lock);
		if(!bitmap)
			return 0;

		mutexAcquire(&bitmap->lock);
		const size_t pages = (bitmap->end - bitmap->base) / G_PAGE_SIZE;
		const size_t words = (pages + G_BITMAP_BITS_PER_ENTRY - 1) / G_BITMAP_BITS_PER_ENTRY;
		for(size_t entryIndex = allocator->firstWord[bitmapIndex]; entryIndex < words; ++entryIndex)
		{
			const uint64_t entry = bitmap->entries[entryIndex];
			if(entry == 0)
				continue;

			const uint64_t remaining = entry & (entry - 1);
			bitmap->entries[entryIndex] = remaining;
			allocator->firstWord[bitmapIndex] = entryIndex + (remaining == 0);
			const size_t bitIndex = entryIndex * G_BITMAP_BITS_PER_ENTRY + __builtin_ctzll(entry);
			const auto address = bitmap->base + bitIndex * G_PAGE_SIZE;
			mutexRelease(&bitmap->lock);
			return address;
		}
		allocator->firstWord[bitmapIndex] = words;

		mutexAcquire(&allocator->fastBuffer.lock);
		mutexAcquire(&allocator->lock);

		// Another allocator may already have advanced, or a free may have
		// reopened an earlier bitmap. Only advance the bound we observed empty.
		if(allocator->firstBitmap == bitmapIndex)
			++allocator->firstBitmap;
		mutexRelease(&bitmap->lock);
	}
}

g_physical_address bitmapPageAllocatorAllocate(g_bitmap_page_allocator* allocator)
{
	auto page = bitmapPageAllocatorTryAllocate(allocator, 0);
	if(!page)
		panic("%! failed to allocate critical physical memory", "bitmap");
	return page;
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
