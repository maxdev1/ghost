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

#ifndef GHOST_SHARED_MEMORY_GDT_GDT
#define GHOST_SHARED_MEMORY_GDT_GDT

#include "ghost/stdint.h"
#include <memory/gdt/gdt.hpp>
#include <memory/gdt/gdt_pointer.hpp>
#include <memory/gdt/gdt_entry.hpp>
#include <memory/gdt/gdt_macros.hpp>

/**
 * Class for writing values to a GDT entry
 */
class g_gdt {
public:

	/**
	 * Fills the given GDTEntry with the given data.
	 *
	 * @param gdtEntry		the target GDT entry
	 * @param base			the base address to write
	 * @param limit			the limit to write
	 * @param access		the access flag to write
	 * @param granularity	the granularity to write
	 */
	static void createGate(g_gdt_entry* gdtEntry, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);

};

#endif
