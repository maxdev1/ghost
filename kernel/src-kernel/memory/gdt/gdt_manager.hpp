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

#ifndef GHOST_GDT_INITIALIZER
#define GHOST_GDT_INITIALIZER

#include "ghost/stdint.h"
#include <memory/gdt/gdt.hpp>
#include <memory/gdt/tss.hpp>
#include <memory/paging.hpp>
#include <memory/memory.hpp>

/**
 * Number of entries in a GDT
 */
#define G_GDT_NUM_ENTRIES 7

/**
 *
 */
struct g_gdt_list_entry {
	g_gdt_pointer ptr;
	g_gdt_entry entry[G_GDT_NUM_ENTRIES];
	g_tss tss;
};

/**
 * GDT initialization manager
 */
class g_gdt_manager {
private:
	/**
	 *
	 */
	static g_gdt_list_entry* getGdtForCore(uint32_t coreId);

public:

	/**
	 * Creates the array of GDT-to-core assignment
	 * structs. Only called by the BSP.
	 */
	static void prepare();

	/**
	 * Initializes the local GDT.
	 */
	static void initialize();

	/**
	 *
	 */
	static void setTssEsp0(g_virtual_address esp0);

	/**
	 *
	 */
	static void setUserThreadAddress(g_virtual_address user_thread_addr);

};

#endif
