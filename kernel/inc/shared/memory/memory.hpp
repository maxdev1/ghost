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

#ifndef __MEMORY__
#define __MEMORY__

#include "shared/memory/bitmap_page_allocator.hpp"
#include <ghost/stdint.h>
#include <stddef.h>

#define G_ALIGN_UP(value, boundary)    (((value) + ((boundary) - 1)) & ~((boundary) - 1))
#define G_ALIGN_DOWN(value, boundary)  ((value) & ~((boundary) - 1))

/**
 * Reference to the loaders or kernels physical page allocator.
 */
extern g_bitmap_page_allocator memoryPhysicalAllocator;

/**
 * Sets number bytes at target to value.
 *
 * @param target	the target pointer
 * @param value		the byte value
 * @param number	the number of bytes to set
 */
void* memorySetBytes(void* target, uint8_t value, int32_t number);

/**
 * Sets number words at target to value.
 *
 * @param target	the target pointer
 * @param value		the word value
 * @param number	the number of word to set
 */
void* memorySetWords(void* target, uint16_t value, int32_t number);

/**
 * Copys size bytes from source to target.
 *
 * @param source	pointer to the source memory location
 * @param target	pointer to the target memory location
 * @param size		number of bytes to copy
 */
void* memoryCopy(void* target, const void* source, int32_t size);

#endif
