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

#ifndef GHOST_KERNEL_MEMORY_BITMAPPHYSICALPAGEALLOCATOR
#define GHOST_KERNEL_MEMORY_BITMAPPHYSICALPAGEALLOCATOR

#include "ghost/stdint.h"
#include <memory/bitmap/bitmap.hpp>
#include <memory/bitmap/bitmap_page_allocator.hpp>

/**
 * Reads the GRUB memory map
 */
class g_multiboot_mmap_interpreter {
public:

	/**
	 * Reads the GRUB memory map to find out which memory areas are usable and free.
	 * Excludes everything before "reservedAreaEnd" and also excludes the locations
	 * of the multiboot modules.
	 */
	static void load(g_bitmap_page_allocator* allocator, uint32_t reservedAreaEnd);
};

#endif
