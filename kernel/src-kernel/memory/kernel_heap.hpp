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

#ifndef MEMORY_GHOST_HEAP_ALLOCATOR
#define MEMORY_GHOST_HEAP_ALLOCATOR

#include <stddef.h>
#include "ghost/stdint.h"
#include <memory/paging.hpp>
#include <memory/memory.hpp>

// 1 MiB
#define G_KERNEL_HEAP_EXPAND_STEP	0x100000

/**
 * The kernel heap is the kernel space memory manager. It is initialized
 * on startup and is used in all calls to new/delete within the kernel.
 */
class g_kernel_heap {
public:

	/**
	 * Initializes the kernel heap using the given range of memory.
	 *
	 * @param start		the start address
	 * @param end		the end address
	 */
	static void initialize(g_virtual_address start, g_virtual_address end);

	/**
	 * Expands the heap space by {G_KERNEL_HEAP_EXPAND_STEP} bytes.
	 *
	 * @return whether the operation was successful
	 */
	static bool expandHeap();
	static void dump();

	static void* allocate(uint32_t size);
	static void free(void* memory);

	static uint32_t getUsedMemoryAmount();
};

#endif
