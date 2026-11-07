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

#ifndef __GHOST_LIBAPI_MALLOC__
#define __GHOST_LIBAPI_MALLOC__

#include "ghost/common.h"
#include <stddef.h>

#define _GLIBCXX_HAVE_ALIGNED_ALLOC 1

__BEGIN_C

/**
 * Releases memory obtained from a compatible C allocation function.
 *
 * After the call, ptr and pointers into the allocation must no longer be used.
 *
 * @param[in] ptr Allocation to release, or null to do nothing.
 * @security-level APPLICATION
 */
void free(void* ptr);

/**
 * Allocates uninitialized memory from the process heap.
 *
 * This header declares the C allocator interface; its implementation is supplied by the linked
 * allocator.
 *
 * @param[in] size Number of bytes to allocate.
 * @return An allocation to release with free(), or null on failure.
 * @security-level APPLICATION
 */
void* malloc(size_t size);

/**
 * Allocates memory at the requested alignment.
 *
 * The linked allocator must supply this entry point; libapi only declares it.
 *
 * @param[in] alignment Supported power-of-two alignment in bytes.
 * @param[in] size Number of bytes to allocate; must be a multiple of alignment.
 * @return An aligned allocation to release with free(), or null on failure.
 * @security-level APPLICATION
 */
void* aligned_alloc(size_t alignment, size_t size);

/**
 * Allocates an array and initializes all its bytes to zero.
 *
 * @param[in] num Number of elements.
 * @param[in] size Size of each element in bytes.
 * @return An allocation to release with free(), or null on failure, including size overflow.
 * @security-level APPLICATION
 */
void* calloc(size_t num, size_t size);

/**
 * Changes the size of a heap allocation, moving it if necessary.
 *
 * Preserves the first min(old size, size) bytes. After success, use only the returned pointer.
 * Zero-size behavior depends on the linked allocator; use free() to release a block explicitly.
 *
 * @param[in] ptr Allocation to resize, or null to allocate a new block.
 * @param[in] size Requested allocation size in bytes.
 * @return The resized allocation, or null on failure; a failed nonzero-size request leaves ptr
 *         valid.
 * @security-level APPLICATION
 */
void* realloc(void* ptr, size_t size);

__END_C

#endif
