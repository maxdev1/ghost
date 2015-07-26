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

#ifndef GHOST_SHARED_MEMORY_BITMAPPHYSICALPAGEALLOCATOR
#define GHOST_SHARED_MEMORY_BITMAPPHYSICALPAGEALLOCATOR

#include "ghost/stdint.h"
#include <memory/bitmap/bitmap.hpp>

/**
 * Physical page allocator implementation using a bitmap
 */
class g_bitmap_page_allocator {
private:
	g_bitmap_entry* bitmap;

public:

	/**
	 *
	 */
	void initialize(g_bitmap_entry* bitmapAddress);

	/**
	 * Marks the given page-aligned address as free in the bitmap.
	 *
	 * @param address	the address of the page to free
	 */
	void markFree(uint32_t address);

	/**
	 * Returns the next free page.
	 *
	 * @return address of a free page
	 */
	uint32_t allocate();
};

#endif
