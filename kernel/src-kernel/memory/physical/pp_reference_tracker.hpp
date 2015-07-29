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

#ifndef PHYSICALPAGESHARETRACKER_HPP_
#define PHYSICALPAGESHARETRACKER_HPP_

#include "ghost/stdint.h"
#include "ghost/kernel.h"
#include "ghost/types.h"

/**
 *
 */
struct g_pp_reference_count_table {
	int16_t referenceCount[1024];
};

/**
 *
 */
struct g_pp_reference_count_directory {
	g_pp_reference_count_table* tables[1024];
};

/**
 * Keeps track of the number of processes that reference pages
 * either in their process image or heap area.
 */
class g_pp_reference_tracker {
public:

	/**
	 *
	 */
	static void increment(g_physical_address address);

	/**
	 *
	 */
	static int16_t decrement(g_physical_address address);

};

#endif
