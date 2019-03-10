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

#include "shared/runtime/constructors.hpp"
#include "shared/logger/logger.hpp"

/**
 * Definition of a constructor
 */
typedef void (*g_constructor)();

/**
 * Constructor symbols, defined in the kernel linker script. Please note that these
 * variables are not actually pointers, but symbols of no type (only use their
 * address by referencing).
 */
extern "C"
{
void* startConstructors;
void* endConstructors;
}

void runtimeAbiCallGlobalConstructors()
{
	g_constructor* start = (g_constructor*) &startConstructors;
	g_constructor* end = (g_constructor*) &endConstructors;

	for(g_constructor* current = start; current != end; ++current)
	{
		(*current)();
	}
}

