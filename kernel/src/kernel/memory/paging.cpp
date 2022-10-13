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

#include "kernel/memory/paging.hpp"
#include "kernel/memory/address_range_pool.hpp"
#include "kernel/memory/memory.hpp"
#include "loader/setup_information.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"
#include "shared/memory/constants.hpp"
#include "shared/panic.hpp"

void pagingMapToTemporaryMappedDirectory(g_physical_address directoryPhys, g_virtual_address virt, g_physical_address phys, uint32_t tableFlags,
										 uint32_t pageFlags, bool allowOverride)
{
	if(!memoryVirtualRangePool)
		panic("%! kernel virtual address range pool used before initialization", "paging");

	if((virt & G_PAGE_ALIGN_MASK) || (phys & G_PAGE_ALIGN_MASK))
		panic("%! tried to map unaligned addresses: %h -> %h", "paging", virt, phys);

	g_virtual_address directoryTempVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	g_page_directory directoryTemp = (g_page_directory) directoryTempVirt;
	pagingMapPage(directoryTempVirt, directoryPhys);

	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virt);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virt);

	// Create table if necessary
	if(directoryTemp[ti] == 0)
	{
		g_physical_address tablePhys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		g_virtual_address tableTempVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
		pagingMapPage(tableTempVirt, tablePhys);

		g_page_table tableTemp = (g_page_table) tableTempVirt;
		for(uint32_t i = 0; i < 1024; i++)
			tableTemp[i] = 0;

		pagingUnmapPage(tableTempVirt);
		addressRangePoolFree(memoryVirtualRangePool, tableTempVirt);

		directoryTemp[ti] = tablePhys | tableFlags;
	}

	// Insert address into table
	g_physical_address tablePhys = (directoryTemp[ti] & ~G_PAGE_ALIGN_MASK);
	g_virtual_address tableTempVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	pagingMapPage(tableTempVirt, tablePhys);

	g_page_table tableTemp = (g_page_table) tableTempVirt;
	if(tableTemp[pi] == 0 || allowOverride)
	{
		tableTemp[pi] = phys | pageFlags;

		addressRangePoolFree(memoryVirtualRangePool, tableTempVirt);
		addressRangePoolFree(memoryVirtualRangePool, directoryTempVirt);
		return;
	}

	logWarn("%! tried to map area to physical pd %h that was already mapped, %h -> %h, table contains %h", "addrspace", directoryPhys, virt, phys,
			tableTemp[pi]);
	panic("%! duplicate mapping", "paging");
}

g_physical_address pagingVirtualToPhysical(g_virtual_address addr)
{
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(addr);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(addr);

	g_page_directory directory = (g_page_directory) G_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	if(directory[ti] == 0)
		return 0;

	g_page_table table = ((g_page_table) G_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);
	return table[pi] & ~G_PAGE_ALIGN_MASK;
}
