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

#include <memory/physical/pp_reference_tracker.hpp>
#include <memory/paging.hpp>
#include <logger/logger.hpp>

g_pp_reference_count_directory directory;

/**
 *
 */
void g_pp_reference_tracker::increment(g_physical_address address) {

	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(address);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(address);

	if (directory.tables[ti] == 0) {
		directory.tables[ti] = new g_pp_reference_count_table;

		for (uint32_t i = 0; i < 1024; i++) {
			directory.tables[ti]->referenceCount[i] = 0;
		}
	}

	++(directory.tables[ti]->referenceCount[pi]);
}

/**
 *
 */
int16_t g_pp_reference_tracker::decrement(g_physical_address address) {

	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(address);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(address);

	if (directory.tables[ti] == 0) {
		return 0;
	}

	return --(directory.tables[ti]->referenceCount[pi]);
}
