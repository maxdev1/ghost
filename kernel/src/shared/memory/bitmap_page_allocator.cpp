/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

void bitmapPageAllocatorFastBufferInitialize(g_bitmap_page_allocator* allocator);
bool bitmapPageAllocatorFastBufferFree(g_bitmap_page_allocator* allocator, g_physical_address address);
g_physical_address bitmapPageAllocatorFastBufferAllocate(g_bitmap_page_allocator* allocator);


void bitmapPageAllocatorInitialize(g_bitmap_page_allocator* allocator, g_bitmap_header* bitmapArray)
{
	allocator->bitmapArray = bitmapArray;
	allocator->freePageCount = 0;

	g_bitmap_header* bitmap = bitmapArray;
	while(bitmap)
	{
		mutexInitializeGlobal(&bitmap->lock);
		allocator->freePageCount += bitmap->entryCount * G_BITMAP_PAGES_PER_ENTRY;
		bitmap = G_BITMAP_NEXT(bitmap);
	}

	bitmapPageAllocatorFastBufferInitialize(allocator);
}


void bitmapPageAllocatorRelocate(g_bitmap_page_allocator* allocator, g_virtual_address newBitmapArray)
{
	allocator->bitmapArray = (g_bitmap_header*) newBitmapArray;
}

void bitmapPageAllocatorMarkFree(g_bitmap_page_allocator* allocator, g_physical_address address)
{
	if(bitmapPageAllocatorFastBufferFree(allocator, address))
		return;

	bool freed = false;

	g_bitmap_header* bitmap = allocator->bitmapArray;
	while(bitmap)
	{
		mutexAcquire(&bitmap->lock);

		g_address endAddress = bitmap->baseAddress + (bitmap->entryCount * G_BITMAP_PAGES_PER_ENTRY) * G_PAGE_SIZE;
		if(address >= bitmap->baseAddress && address < endAddress)
		{
			g_offset offset = address - bitmap->baseAddress;

			uint32_t index = G_OFFSET_TO_BITMAP_INDEX(offset);
			uint32_t bit = G_OFFSET_TO_BITMAP_BIT(offset);
			G_BITMAP_UNSET(bitmap, index, bit);

			if(index < bitmap->firstFree)
				bitmap->firstFree = index;

			++allocator->freePageCount;

			freed = true;
		}

		mutexRelease(&bitmap->lock);

		if(freed)
			break;

		bitmap = G_BITMAP_NEXT(bitmap);
	}

	if(!freed)
		logWarn("%! failed to free physical address %x", "bitmap", address);
}

g_physical_address bitmapPageAllocatorAllocate(g_bitmap_page_allocator* allocator)
{
	g_physical_address addressFromFB = bitmapPageAllocatorFastBufferAllocate(allocator);
	if(addressFromFB)
		return addressFromFB;

	g_bitmap_header* bitmap = allocator->bitmapArray;
	while(bitmap)
	{
		g_physical_address result = 0;
		mutexAcquire(&bitmap->lock);

		for(uint32_t i = bitmap->firstFree; i < bitmap->entryCount; i++)
		{
			if(G_BITMAP_ENTRIES(bitmap)[i] == G_BITMAP_ENTRY_FULL)
				continue;

			for(uint32_t b = 0; b < G_BITMAP_PAGES_PER_ENTRY; b++)
			{
				if(G_BITMAP_IS_SET(bitmap, i, b))
					continue;

				G_BITMAP_SET(bitmap, i, b);
				--allocator->freePageCount;

				result = bitmap->baseAddress + G_BITMAP_TO_OFFSET(i, b);
				break;
			}

			if(result)
				break;
		}

		mutexRelease(&bitmap->lock);
		if(result)
			return result;

		bitmap = G_BITMAP_NEXT(bitmap);
	}

	logWarn("%! failed to allocate physical page", "bitmap");
	return 0;
}

/**
 * Initializes the "fast buffer" which is used to keep a stack of free pages for faster allocation.
 *
 * @param allocator
 */
void bitmapPageAllocatorFastBufferInitialize(g_bitmap_page_allocator* allocator)
{
	mutexInitializeGlobal(&allocator->fastBuffer.lock);
	for(g_physical_address& cell: allocator->fastBuffer.buffer)
	{
		cell = 0;
	}
	allocator->fastBuffer.size = 0;
}

bool bitmapPageAllocatorFastBufferFree(g_bitmap_page_allocator* allocator, g_physical_address address)
{
	bool freed = false;

	mutexAcquire(&allocator->fastBuffer.lock);
	if(allocator->fastBuffer.size < G_BITMAP_ALLOCATOR_FASTBUFFER_SIZE)
	{
		allocator->fastBuffer.buffer[allocator->fastBuffer.size++] = address;
		freed = true;
	}
	mutexRelease(&allocator->fastBuffer.lock);

	return freed;
}

g_physical_address bitmapPageAllocatorFastBufferAllocate(g_bitmap_page_allocator* allocator)
{
	g_physical_address address = 0;

	mutexAcquire(&allocator->fastBuffer.lock);
	if(allocator->fastBuffer.size > 0)
	{
		address = allocator->fastBuffer.buffer[--allocator->fastBuffer.size];
	}
	mutexRelease(&allocator->fastBuffer.lock);

	return address;
}
