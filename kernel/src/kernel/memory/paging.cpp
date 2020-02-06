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
#include "kernel/kernel.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/address_range_pool.hpp"

#include "shared/memory/constants.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"

bool pagingMapPage(g_virtual_address virt, g_physical_address phys, uint32_t tableFlags, uint32_t pageFlags, bool allowOverride)
{
	if((virt & G_PAGE_ALIGN_MASK) || (phys & G_PAGE_ALIGN_MASK))
		kernelPanic("%! tried to map unaligned addresses: %h -> %h", "paging", virt, phys);

	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virt);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virt);

	g_page_directory directory = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	g_page_table table = ((g_page_table) G_CONST_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);

	if(directory[ti] == 0)
	{
		g_physical_address newTablePage = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		if(!newTablePage)
			kernelPanic("%! no pages left for mapping", "paging");

		directory[ti] = newTablePage | tableFlags;
		for(uint32_t i = 0; i < 1024; i++)
			table[i] = 0;

	} else if((tableFlags & G_PAGE_TABLE_USERSPACE) && ((directory[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE) == 0)
	{
		kernelPanic("%! tried to map user page in kernel space table, virt %h", "paging", virt);
	}

	if(table[pi] == 0 || allowOverride)
	{
		table[pi] = phys | pageFlags;
		G_INVLPG(virt);
		return true;
	}

#warning "TODO: implement following code"
	logInfo("%! warning: tried duplicate mapping of page %h", "paging", virt);
	/*
	 g_thread* failor = g_tasking::lastThread();
	 if(failor != 0)
	 {
	 const char* ident = failor->getIdentifier();
	 if(ident)
	 {
	 logInfo("%! '%s' (%i) tried duplicate mapping, virt %h -> phys %h, table contains %h", "addrspace", ident, failor->id, virtual_addr,
	 physical_addr, table[pi]);
	 } else
	 {
	 logInfo("%! %i tried duplicate mapping, virt %h -> phys %h, table contains %h", "addrspace", failor->id, virtual_addr, physical_addr, table[pi]);
	 }
	 } else
	 {
	 logInfo("%! unknown tried duplicate mapping, virt %h -> phys %h, table contains %h", "addrspace", virtual_addr, physical_addr, table[pi]);
	 }
	 */
	return false;
}

void pagingMapToTemporaryMappedDirectory(g_physical_address directoryPhys, g_virtual_address virt, g_physical_address phys, uint32_t tableFlags,
		uint32_t pageFlags, bool allowOverride)
{
	if(!memoryVirtualRangePool)
		kernelPanic("%! kernel virtual address range pool used before initialization", "paging");

	if((virt & G_PAGE_ALIGN_MASK) || (phys & G_PAGE_ALIGN_MASK))
		kernelPanic("%! tried to map unaligned addresses: %h -> %h", "paging", virt, phys);

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
	kernelPanic("%! duplicate mapping", "paging");
}

void pagingUnmapPage(g_virtual_address virt)
{
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virt);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virt);

	g_page_directory directory = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	g_page_table table = G_CONST_RECURSIVE_PAGE_TABLE(ti);

	if(!directory[ti])
		return;

	if(!table[pi])
		return;

	table[pi] = 0;
	G_INVLPG(virt);
}

g_physical_address pagingGetCurrentSpace()
{
	uint32_t directory;
	asm volatile("mov %%cr3, %0" : "=r"(directory));
	return directory;
}

g_physical_address pagingVirtualToPhysical(g_virtual_address addr)
{
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(addr);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(addr);
	g_page_directory directory = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	g_page_table table = ((g_page_table) G_CONST_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);

	if(directory[ti] == 0)
		return 0;

	return table[pi] & ~G_PAGE_ALIGN_MASK;
}

