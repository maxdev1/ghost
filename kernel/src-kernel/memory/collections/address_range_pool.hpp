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

#ifndef GHOST_MEMORY_ADDRESS_RANGE_POOL
#define GHOST_MEMORY_ADDRESS_RANGE_POOL

#include <memory/memory.hpp>

/**
 * An address range is a range of pages starting at a base. The base
 * determines the address of the range, and pages is the number of
 * pages the range has.
 */
struct g_address_range {

	g_address_range() :
			next(0), used(false), base(0), pages(0), flags(0) {
	}

	g_address_range* next;

	bool used;
	g_address base;
	uint32_t pages;

	uint8_t flags;
};

/**
 * A address range pool manages ranges of page-aligned virtual
 * addresses.
 */
class g_address_range_pool {
private:
	g_address_range* first;

public:
	g_address_range_pool() :
			first(0) {
	}
	~g_address_range_pool();

	g_address allocate(uint32_t pages, uint8_t flags = 0);
	int32_t free(g_address base);

	g_address_range* getRanges();

	/**
	 * Initialize from ranges
	 */
	void initialize(g_address start, g_address end);

	/**
	 * Initialize by clone
	 */
	void initialize(g_address_range_pool* other);

	void dump(bool onlyFree = false);

private:
	void merge();
};

#endif
