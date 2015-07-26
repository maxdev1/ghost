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

#ifndef MEMORY_LOWERMEM_ALLOC
#define MEMORY_LOWERMEM_ALLOC

#include <stddef.h>
#include "ghost/stdint.h"

/**
 *
 */
struct g_lower_heap_header {
	g_lower_heap_header* next;
	uint8_t used;
	uint32_t size;
};

/**
 *
 */
class g_lower_heap {
public:
	static void addArea(uint32_t start, uint32_t end);
	static void* allocate(int32_t size);
	static void free(void* memory);
	static void merge();

	static void dump();
};

#endif
