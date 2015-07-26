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

#ifndef GHOST_LOADER_MEMORY_GDT_GDT_INITIALIZER
#define GHOST_LOADER_MEMORY_GDT_GDT_INITIALIZER

#include "ghost/stdint.h"
#include <memory/gdt/gdt.hpp>

/**
 * Initializer class for the global descriptor table
 */
class g_gdt_manager {
public:

	/**
	 * Creates the GDT in the page addressed by the usableAddress.
	 *
	 * @param usableAddress		the address of the page that can be used to
	 * 							store the GDT pointer and the GDT
	 */
	static void initialize(uint32_t usableAddress);

};

#endif
