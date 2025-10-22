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

#ifndef __KERNEL_HEAP__
#define __KERNEL_HEAP__

#include <ghost/memory/types.h>

/**
 * Initializes the kernel heap. This maps an initial memory area and
 * then performs an initialization of a memory allocator on this area.
 */
void heapInitialize();

/**
 * Allocates a number of bytes on the kernel heap.
 *
 * Causes a panic if it fails.
 */
void* heapAllocate(uint32_t size);

/**
 * Allocates a number of bytes on the kernel heap and clears it.
 *
 * Causes a panic if it fails.
 */
void* heapAllocateClear(uint32_t size);

/**
 * Frees the given range.
 */
void heapFree(void* memory);

/**
 * Returns the amount of used bytes in the kernel heap.
 */
uint32_t heapGetUsedAmount();

#endif
