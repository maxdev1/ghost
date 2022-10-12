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

#ifndef __KERNEL_PAGING__
#define __KERNEL_PAGING__

#include "ghost/stdint.h"
#include "shared/memory/memory.hpp"
#include "shared/memory/paging.hpp"

/**
 * Used to map a page into a page directory that is not the current address space.
 * The directory itself and the required tables are temporarily mapped and then unmapped again.
 */
void pagingMapToTemporaryMappedDirectory(g_physical_address directory, g_virtual_address virtualAddress, g_physical_address physicalAddress,
										 uint32_t tableFlags =
											 DEFAULT_KERNEL_TABLE_FLAGS,
										 uint32_t pageFlags = DEFAULT_KERNEL_PAGE_FLAGS, bool allowOverride = false);

/**
 * Reads for a given virtual address (which must exist in the currently mapped
 * address space) the underlying physical address.
 *
 * @param addr
 * 		the address to resolve
 *
 * @return the physical address
 */
g_physical_address pagingVirtualToPhysical(g_virtual_address addr);

#endif
