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

#ifndef GHOST_MEMORY_ADDRESS_SPACE
#define GHOST_MEMORY_ADDRESS_SPACE

#include "ghost/stdint.h"
#include <memory/paging.hpp>
#include <memory/memory.hpp>
#include <memory/address_space.hpp>

/**
 * Functionality to manipulate the address space.
 */
class g_address_space {
public:

	/**
	 * Maps a page to the current address space.
	 *
	 * @param virt
	 * 		the virtual address to map to
	 * @param phys
	 * 		the address of the physical page to map
	 * @param table_flags
	 * 		the flags to add on the table entry
	 * @param page_flags
	 * 		the flags to add on the page entry
	 * @param allow_override
	 * 		whether an existing entry may be overriden
	 *
	 *
	 */
	static bool map(g_virtual_address virt, g_physical_address phys, uint32_t table_flags, uint32_t page_flags, bool allow_override = false);

	/**
	 * Maps a page to a page directory that is temporarily mapped into the current address space.
	 *
	 * @param directory
	 * 		the directory to map into
	 * @other-params see map
	 */
	static void map_to_temporary_mapped_directory(g_page_directory directory, g_virtual_address virtualAddress, g_physical_address physicalAddress,
			uint32_t tableFlags, uint32_t pageFlags, bool allow_override = false);

	/**
	 * Unmaps the given virtual page in the current address space.
	 *
	 * @param virt
	 * 		the virtual address to unmap
	 */
	static void unmap(g_virtual_address virt);

	/**
	 * Switches to the given page directory.
	 *
	 * @param dir
	 * 		the directory to switch to
	 */
	static void switch_to_space(g_page_directory dir);

	/**
	 * Returns the currently set page directory.
	 *
	 * @return the page directory
	 */
	static g_page_directory get_current_space();

	/**
	 * Reads for a given virtual address (which must exist in the currently mapped
	 * address space) the underlying physical address.
	 *
	 * @param addr
	 * 		the address to resolve
	 *
	 * @return the physical address
	 */
	static g_physical_address virtual_to_physical(g_virtual_address addr);

};

#endif
