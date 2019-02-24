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

#ifndef __GDT__
#define __GDT__

#include "ghost/stdint.h"
#include "shared/memory/gdt_pointer.hpp"
#include "shared/memory/gdt_entry.hpp"
#include "shared/memory/gdt_macros.hpp"

/**
 * Fills the given GDTEntry with the given data.
 *
 * @param gdtEntry		the target GDT entry
 * @param base			the base address to write
 * @param limit			the limit to write
 * @param access		the access flag to write
 * @param granularity	the granularity to write
 */
void gdtCreateGate(g_gdt_entry* gdtEntry, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);

#endif
