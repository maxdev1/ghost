/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2024, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __KERNEL_MEMORY_ALLOCATOR__
#define __KERNEL_MEMORY_ALLOCATOR__

#include "shared/logger/logger.hpp"
#include "shared/system/mutex.hpp"
#include <ghost/types.h>

/**
 * Types of allocators
 */
#define G_ALLOCATOR_TYPE_HEAP 0
#define G_ALLOCATOR_TYPE_LOWERMEM 1
typedef uint32_t g_allocator_type;

/**
 * Any allocation larger than this will be created as a chunk and not within buckets
 */
#define G_ALLOCATOR_MAX_FOR_BUCKETS 1024

/**
 * Type of allocated sections in the used memory range
 */
#define G_ALLOCATOR_SECTION_TYPE_FREE 0
#define G_ALLOCATOR_SECTION_TYPE_CHUNK 1
#define G_ALLOCATOR_SECTION_TYPE_BUCKET 2
typedef uint32_t g_allocator_section_type;

/**
 * Header of an allocation section
 */
struct g_allocator_section_header
{
	g_allocator_section_type type;
	g_allocator_section_header* next;
	g_size totalSize; // including header
};

/**
 * Bucket starts with the same header but then adds additional fields to the in-memory structure
 */
struct g_allocator_section_bucket
{
	g_allocator_section_header header;
	uint16_t entrySize;
	uint16_t bitmapSize;
	// uint8_t[bitmapSize] bitmap
	// uint8_t[entrySize * bitmapSize * 8] bucketContent
};

/**
 * Memory layout of the in-place allocator
 */
struct g_allocator
{
	g_mutex lock;
	g_allocator_type type;
	g_allocator_section_header* sections;
};

/**
 * Initializes a chunk allocator in the given range.
 */
void memoryAllocatorInitialize(g_allocator* allocator, g_allocator_type type, g_virtual_address start, g_virtual_address end);

/**
 * Expands the range that the allocator uses by the given amount of bytes.
 */
void memoryAllocatorExpand(g_allocator* allocator, g_size size);

/**
 * Allocates memory using an allocator.
 */
void* memoryAllocatorAllocate(g_allocator* allocator, g_size size);

/**
 * Frees memory using an allocator.
 */
g_size memoryAllocatorFree(g_allocator* allocator, void* memory);

#endif
