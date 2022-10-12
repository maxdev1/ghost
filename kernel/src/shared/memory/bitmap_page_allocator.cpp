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
#include "shared/memory/paging.hpp"
#include "shared/multiboot/multiboot.hpp"

void bitmapPageAllocatorInitialize(g_bitmap_page_allocator* allocator, g_bitmap* bitmapArray)
{
	allocator->bitmapArray = bitmapArray;
	allocator->freePageCount = 0;
	mutexInitialize(&allocator->lock);

	g_bitmap* bitmap = bitmapArray;
	while(bitmap)
	{
		allocator->freePageCount += bitmap->entryCount * G_BITMAP_PAGES_PER_ENTRY;
		bitmap = G_BITMAP_NEXT(bitmap);
	}
}

void bitmapPageAllocatorRelocate(g_bitmap_page_allocator* allocator, g_virtual_address newBitmapArray)
{
	allocator->bitmapArray = (g_bitmap*) newBitmapArray;
}

void bitmapPageAllocatorMarkFree(g_bitmap_page_allocator* allocator, g_physical_address address)
{
	mutexAcquire(&allocator->lock);
	bool freed = false;

	g_bitmap* bitmap = allocator->bitmapArray;
	while(bitmap)
	{
		g_address endAddress = bitmap->baseAddress + (bitmap->entryCount * G_BITMAP_PAGES_PER_ENTRY) * G_PAGE_SIZE;
		if(address >= bitmap->baseAddress && address < endAddress)
		{
			g_offset offset = address - bitmap->baseAddress;

			uint32_t index = G_OFFSET_TO_BITMAP_INDEX(offset);
			uint32_t bit = G_OFFSET_TO_BITMAP_BIT(offset);
			G_BITMAP_UNSET(bitmap, index, bit);

			++allocator->freePageCount;

			freed = true;
			break;
		}

		bitmap = G_BITMAP_NEXT(bitmap);
	}

	if(!freed)
		logWarn("%! failed to free physical address %x", "bitmap", address);

	mutexRelease(&allocator->lock);
}

g_physical_address bitmapPageAllocatorAllocate(g_bitmap_page_allocator* allocator)
{
	mutexAcquire(&allocator->lock);

	g_bitmap* bitmap = allocator->bitmapArray;
	while(bitmap)
	{
		for(uint32_t i = 0; i < bitmap->entryCount; i++)
		{
			if(G_BITMAP_ENTRIES(bitmap)[i] == G_BITMAP_ENTRY_FULL)
				continue;

			for(uint32_t b = 0; b < G_BITMAP_PAGES_PER_ENTRY; b++)
			{
				if(G_BITMAP_IS_SET(bitmap, i, b))
					continue;

				G_BITMAP_SET(bitmap, i, b);
				--allocator->freePageCount;

				g_physical_address result = bitmap->baseAddress + G_BITMAP_TO_OFFSET(i, b);
				mutexRelease(&allocator->lock);
				return result;
			}
		}

		bitmap = G_BITMAP_NEXT(bitmap);
	}

	mutexRelease(&allocator->lock);

	logWarn("%! failed to allocate physical page", "bitmap");
	return 0;
}
