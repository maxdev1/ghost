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

#include <memory/allocators/chunk_allocator.hpp>
#include <logger/logger.hpp>

/**
 *
 */
void g_chunk_allocator::initialize(g_virtual_address start, g_virtual_address end) {
	first = (g_chunk_header*) start;
	first->used = false;
	first->size = end - start - sizeof(g_chunk_header);
	first->next = 0;
}

/**
 *
 */
void g_chunk_allocator::expand(g_virtual_address size) {

	// Find last chunk
	g_chunk_header* last = first;
	while (last->next) {
		last = last->next;
	}

	// Create appendix
	g_chunk_header* appendix = (g_chunk_header*) (((uint32_t) last) + sizeof(g_chunk_header) + last->size);
	appendix->used = false;
	appendix->size = size - sizeof(g_chunk_header);
	appendix->next = 0;

	// Append on the end
	last->next = appendix;

	merge();
}

/**
 *
 */
void* g_chunk_allocator::allocate(uint32_t size) {

	if (first == 0) {
		g_log_info("%! critical: tried to use allocate on uninitialized chunk allocator", "chunkalloc");
		return 0;
	}

	if (size < CHUNK_ALLOCATOR_MINIMUM_ALLOCATION_SIZE) {
		size = CHUNK_ALLOCATOR_MINIMUM_ALLOCATION_SIZE;
	}

	g_chunk_header* current = first;
	do {
		if (!current->used && (current->size >= (size + sizeof(g_chunk_header)))) {

			g_chunk_header* splinter = (g_chunk_header*) ((uint32_t) current + sizeof(g_chunk_header) + size);
			splinter->size = current->size - size - sizeof(g_chunk_header);
			splinter->used = false;
			splinter->next = current->next;

			current->next = splinter;
			current->used = true;
			current->size = size;

			return (void*) (((uint32_t) current) + sizeof(g_chunk_header));
		}
	} while ((current = current->next) != 0);

	return 0;
}

/**
 *
 */
uint32_t g_chunk_allocator::free(void* mem) {
	if (first == 0) {
		g_log_info("%! critical: tried to use free on uninitialized chunk allocator", "chunkalloc");
		return 0;
	}

	g_chunk_header* blockHeader = (g_chunk_header*) (((uint32_t) mem) - sizeof(g_chunk_header));
	blockHeader->used = false;
	uint32_t size = blockHeader->size;

	merge();

	return size;
}

/**
 * Merges contiguous free blocks
 */
void g_chunk_allocator::merge() {

	g_chunk_header* current = (g_chunk_header*) first;

	while (true) {
		// If it has next, continue merging
		if (current->next) {

			// If current is free and next is free, merge them
			if (!(current->used) && !(current->next->used)) {
				current->size += sizeof(g_chunk_header) + current->next->size;
				current->next = current->next->next;
				// Don't step to next here, check this one again for another free following block

			} else {
				// Cannot merge here, step to next
				current = current->next;
			}
		} else {
			break;
		}
	}
}
