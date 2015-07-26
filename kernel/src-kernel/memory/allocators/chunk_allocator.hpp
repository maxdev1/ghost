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

#ifndef CHUNKALLOCATOR_HPP_
#define CHUNKALLOCATOR_HPP_

#include "ghost/stdint.h"
#include "ghost/types.h"
#include <logger/logger.hpp>

#define CHUNK_ALLOCATOR_MINIMUM_ALLOCATION_SIZE	8

/**
 *
 */
struct g_chunk_header {
	g_chunk_header* next;
	uint8_t used;
	uint32_t size;
};

/**
 *
 */
class g_chunk_allocator {
private:
	g_chunk_header* first;
	void merge();

public:
	g_chunk_allocator() :
			first(0) {
	}

	void initialize(g_virtual_address start, g_virtual_address end);
	void expand(g_virtual_address size);

	void* allocate(uint32_t size);
	uint32_t free(void* memory);

};

#endif
