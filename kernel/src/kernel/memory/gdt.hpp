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

#ifndef __KERNEL_GDT__
#define __KERNEL_GDT__

#include "ghost/types.h"
#include "shared/memory/gdt.hpp"
#include "shared/memory/tss.hpp"

#define G_GDT_NUM_ENTRIES 7

struct g_gdt_list_entry
{
	g_gdt_pointer ptr;
	g_gdt_entry entry[G_GDT_NUM_ENTRIES];
	g_tss tss;
};

/**
 * Prepares the global GDT structures.
 */
void gdtPrepare();

/**
 * Initializes the GDT for this core.
 */
void gdtInitialize();

/**
 * Retrieves the GDT for the core with the given id.
 */
g_gdt_list_entry* gdtGetForCore(uint32_t coreId);

/**
 * Sets the ESP0 of the TSS of the current core the given address.
 * The CPU switches to this specified stack before entering an interrupt handler.
 */
void gdtSetTssEsp0(g_virtual_address esp0);

/**
 * For thread local storage, it is needed to write the address of the so-called
 * "user thread" object to the GDT.
 */
void gdtSetUserThreadObjectAddress(g_virtual_address userThreadAddr);

#endif
