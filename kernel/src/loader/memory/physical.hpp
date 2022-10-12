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

#ifndef __LOADER_PHYSICALALLOCATOR__
#define __LOADER_PHYSICALALLOCATOR__

#include "ghost/types.h"
#include "shared/memory/bitmap.hpp"

/**
 * Reads the GRUB memory map to find out which memory areas are usable and free.
 * Everything after "startAfter" is excluded.
 *
 * This function is run twice; the first time, the memory map is interpreted to
 * determine how large all bitmaps in the bitmap array will be. In the second
 * run, a sufficient physical space was allocated and the bitmaps are written
 * to the given address.
 */
uint32_t memoryPhysicalReadMemoryMap(g_address startAfter, g_address bitmapArrayStart);

/**
 * Before the bitmap allocator is initialized, this simple allocation function
 * searches and allocates free pages, starting after the given address.
 */
g_address memoryPhysicalAllocateInitial(g_address startAfter, int pages);

#endif
