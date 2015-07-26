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

#include <memory/lower_heap.hpp>
#include <kernel.hpp>
#include <logger/logger.hpp>
#include <memory/allocators/chunk_allocator.hpp>

static g_chunk_allocator allocator;

/**
 *
 */
void g_lower_heap::addArea(uint32_t start, uint32_t end) {
	allocator.initialize(start, end);
	g_log_debug("%! using area %h to %h", "lowermem", start, end);
}

/**
 *
 */
void* g_lower_heap::allocate(int32_t size) {

	void* allocated = allocator.allocate(size);
	if (allocated) {
		return allocated;
	}

	g_log_info("%! out of memory", "loweralloc");
	return 0;
}

/**
 *
 */
void g_lower_heap::free(void* mem) {
	allocator.free(mem);
}
