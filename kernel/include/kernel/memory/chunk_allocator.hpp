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

#ifndef __KERNEL_CHUNK_ALLOCATOR__
#define __KERNEL_CHUNK_ALLOCATOR__

#include "ghost/types.h"
#include "shared/logger/logger.hpp"
#include "shared/system/mutex.hpp"

#define G_CHUNK_ALLOCATOR_MIN_ALLOC	8

struct g_chunk_header
{
	g_chunk_header* next;
	uint8_t used :1;
	uint32_t size :31;
};

struct g_chunk_allocator
{
	g_mutex lock;
	g_chunk_header* first;
};

/**
 * Initializes a chunk allocator in the given range.
 */
void chunkAllocatorInitialize(g_chunk_allocator* allocator, g_virtual_address start, g_virtual_address end);

/**
 * Expands the range that the allocator uses by the given amount of bytes.
 */
void chunkAllocatorExpand(g_chunk_allocator* allocator, uint32_t size);

void* chunkAllocatorAllocate(g_chunk_allocator* allocator, uint32_t size);

uint32_t chunkAllocatorFree(g_chunk_allocator* allocator, void* memory);

void chunkAllocatorMerge(g_chunk_allocator* allocator);

#endif
