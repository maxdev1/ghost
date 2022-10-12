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

#include "shared/memory/paging.hpp"
#include "shared/memory/constants.hpp"
#include "shared/memory/memory.hpp"
#include "shared/panic.hpp"

void pagingSwitchToSpace(g_physical_address dir)
{
	asm volatile("mov %0, %%cr3" ::"b"(dir));
}

bool pagingMapPage(g_virtual_address virt, g_physical_address phys, uint32_t tableFlags, uint32_t pageFlags, bool allowOverride)
{
	if((virt & G_PAGE_ALIGN_MASK) || (phys & G_PAGE_ALIGN_MASK))
		panic("%! tried to map unaligned addresses: %h -> %h", "paging", virt, phys);

	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virt);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virt);

	g_page_directory directory = (g_page_directory) G_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	g_page_table table = ((g_page_table) G_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);

	if(directory[ti] == 0)
	{
		g_physical_address newTablePage = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		if(!newTablePage)
			panic("%! no pages left for mapping", "paging");

		directory[ti] = newTablePage | tableFlags;
		for(uint32_t i = 0; i < 1024; i++)
			table[i] = 0;
	}
	else if((tableFlags & G_PAGE_TABLE_USERSPACE) && ((directory[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE) == 0)
	{
		panic("%! tried to map user page in kernel space table, virt %h", "paging", virt);
	}

	if(table[pi] == 0 || allowOverride)
	{
		table[pi] = phys | pageFlags;
		G_INVLPG(virt);
		return true;
	}

	logInfo("%! warning: tried duplicate mapping of page %h", "paging", virt);
	return false;
}

void pagingUnmapPage(g_virtual_address virt)
{
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virt);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virt);

	g_page_directory directory = (g_page_directory) G_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	g_page_table table = G_RECURSIVE_PAGE_TABLE(ti);

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
	asm volatile("mov %%cr3, %0"
				 : "=r"(directory));
	return directory;
}
