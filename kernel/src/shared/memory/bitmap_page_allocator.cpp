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

#include "shared/multiboot/multiboot.hpp"
#include "shared/memory/paging.hpp"
#include "shared/logger/logger.hpp"

void bitmapPageAllocatorInitialize(g_bitmap_page_allocator* allocator, g_bitmap_entry* bitmap)
{
	allocator->bitmap = bitmap;
	allocator->freePageCount = 0;
	mutexInitialize(&allocator->lock);

	for(uint32_t i = 0; i < G_BITMAP_SIZE; i++)
		allocator->bitmap[i] = 0;
}

void bitmapPageAllocatorRefresh(g_bitmap_page_allocator* allocator)
{
	for(uint32_t i = 0; i < G_BITMAP_SIZE; i++)
	{
		for(uint8_t b = 0; b < G_BITMAP_BITS_PER_ENTRY; b++)
		{
			if(G_BITMAP_IS_SET(allocator->bitmap, i, b))
			{
				allocator->freePageCount++;
			}
		}
	}
}

void bitmapPageAllocatorMarkFree(g_bitmap_page_allocator* allocator, g_physical_address address)
{
	mutexAcquire(&allocator->lock);

	uint32_t index = G_ADDRESS_TO_BITMAP_INDEX(address);
	uint32_t bit = G_ADDRESS_TO_BITMAP_BIT(address);
	G_BITMAP_SET(allocator->bitmap, index, bit);
	allocator->freePageCount++;

	mutexRelease(&allocator->lock);
}

g_physical_address bitmapPageAllocatorAllocate(g_bitmap_page_allocator* allocator)
{
	mutexAcquire(&allocator->lock);

	for(uint32_t i = 0; i < G_BITMAP_SIZE; i++)
	{
		if(!allocator->bitmap[i])
			continue;

		for(uint32_t b = 0; b < G_BITMAP_BITS_PER_ENTRY; b++)
		{
			if(G_BITMAP_IS_SET(allocator->bitmap, i, b))
			{
				G_BITMAP_UNSET(allocator->bitmap, i, b);
				allocator->freePageCount--;

				mutexRelease(&allocator->lock);
				return G_BITMAP_TO_ADDRESS(i, b);
			}
		}
	}

	mutexRelease(&allocator->lock);
	return 0;
}
