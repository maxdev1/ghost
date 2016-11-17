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

#ifndef __GHOST_LIBC_MALLOC__
#define __GHOST_LIBC_MALLOC__

#include "ghost/common.h"
#include "ghost/malloc.h"
#include <stddef.h>

__BEGIN_C

/**
 * Performs a memory allocation with a size of <size> that is aligned
 * to the <alignment>. (N1548-7.22.3.1)
 *
 * @param alignment
 * 		alignment to use
 * @param size
 * 		size to request
 * @return
 * 		allocated space or 0 if not successful
 */
void* aligned_alloc(size_t alignment, size_t size);

/**
 * Allocates <num> objects of <size> bytes, all bits set to zero. (N1548-7.22.3.2)
 *
 * @param num
 * 		number of objects
 * @param size
 * 		size of each object
 * @return
 * 		allocated space or 0 if not successful
 */
void* calloc(size_t num, size_t size);

/**
 * Reallocates the memory pointed to by <ptr> to have at least
 * <size> bytes after the allocation. (N1548-7.22.3.5)
 *
 * @param ptr
 * 		memory to reallocate
 * @param size
 * 		size in bytes to allocate
 * @return
 * 		allocated memory
 */
void* realloc(void* ptr, size_t size);

__END_C

#endif
