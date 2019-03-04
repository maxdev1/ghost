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

#ifndef __BITMAP_PAGE_ALLOCATOR__
#define __BITMAP_PAGE_ALLOCATOR__

#include "ghost/types.h"
#include "shared/memory/bitmap.hpp"
#include "shared/system/mutex.hpp"

struct g_bitmap_page_allocator
{
	uint32_t freePageCount;
	g_bitmap_entry* bitmap;
	g_mutex lock;
};

void bitmapPageAllocatorInitialize(g_bitmap_page_allocator* allocator, g_bitmap_entry* bitmap);

void bitmapPageAllocatorRefresh(g_bitmap_page_allocator* allocator);

void bitmapPageAllocatorMarkFree(g_bitmap_page_allocator* allocator, g_physical_address address);

g_physical_address bitmapPageAllocatorAllocate(g_bitmap_page_allocator* allocator);

#endif
