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

#include "loader/memory/paging.hpp"
#include "loader/setup_information.hpp"
#include "shared/logger/logger.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"
#include "shared/memory/constants.hpp"
#include "shared/memory/memory.hpp"
#include "shared/memory/paging.hpp"
#include "shared/panic.hpp"

g_physical_address pagingInitialize(g_virtual_address reservedAreaEnd)
{
	g_physical_address pageDirPhys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	if(!pageDirPhys)
		panic("pagingInitialize: failed to allocate memory for page directory");

	g_page_directory pageDirectory = (g_page_directory) pageDirPhys;

	for(uint32_t i = 0; i < 1024; i++)
		pageDirectory[i] = 0;
	pageDirectory[1023] = pageDirPhys | DEFAULT_KERNEL_TABLE_FLAGS;

	pagingIdentityMap(pageDirectory, 0, reservedAreaEnd, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	pagingRelocateMultibootModules(pageDirectory, reservedAreaEnd);
	pagingEnableGlobalPageFlag();
	pagingSwitchToSpace(pageDirPhys);
	pagingEnable();
	return pageDirPhys;
}

void pagingEnableGlobalPageFlag()
{
	uint32_t cr4;
	asm volatile("mov %%cr4, %0"
				 : "=r"(cr4));
	cr4 |= (1 << 7);
	asm volatile("mov %0, %%cr4" ::"b"(cr4));
}

void pagingRelocateMultibootModules(g_page_directory pageDirectory, g_address startAt)
{
	g_multiboot_information* mbInfo = setupInformation.multibootInformation;
	g_address nextModuleLocation = startAt;

	for(int i = 0; i < (int) mbInfo->modulesCount; i++)
	{
		g_multiboot_module* module = &mbInfo->modules[i];
		g_address modPhysStartAligned = G_PAGE_ALIGN_DOWN(module->moduleStart);
		g_address modPhysOff = module->moduleStart - modPhysStartAligned;
		g_address modPhysEndAligned = G_PAGE_ALIGN_UP(module->moduleEnd);
		g_address modPhysLen = module->moduleEnd - module->moduleStart;

		g_address modVirtStartAligned = nextModuleLocation;
		g_address modVirtStart = modVirtStartAligned + modPhysOff;

		for(uint32_t off = 0; off < modPhysEndAligned - modPhysStartAligned; off += G_PAGE_SIZE)
			pagingMapPageToDirectory(pageDirectory, modVirtStartAligned + off, modPhysStartAligned + off,
									 DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);

		// Finish relocation by updating multiboot structure
		logDebugn("%! relocated, mapped phys %h-%h", "mmodule", module->moduleStart, module->moduleEnd);
		module->moduleStart = modVirtStart;
		module->moduleEnd = modVirtStart + modPhysLen;
		logDebug(" to virt %h-%h", module->moduleStart, module->moduleEnd);

		nextModuleLocation = G_PAGE_ALIGN_UP(module->moduleEnd);
	}
}

void pagingIdentityMap(g_page_directory directory, uint32_t start, uint32_t end, uint32_t tableFlags, uint32_t pageFlags)
{
	logDebug("%! identity-mapping: %h - %h", "paging", start, end);

	while(start < end)
	{
		pagingMapPageToDirectory(directory, start, start, tableFlags, pageFlags);
		start += G_PAGE_SIZE;
	}
}

void pagingMapPageToDirectory(g_page_directory directory, g_address virtualAddress, g_address physicalAddress, uint32_t tableFlags, uint32_t pageFlags)
{
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virtualAddress);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virtualAddress);

	if(directory[ti] == 0)
	{
		g_physical_address tablePage = (uint32_t) bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		if(!tablePage)
			panic("pagingMapPage: failed to map %h -> %h, could not allocate page for table", virtualAddress, physicalAddress);

		g_page_table table = (g_page_table) tablePage;
		for(uint32_t i = 0; i < 1024; i++)
			table[i] = 0;

		directory[ti] = tablePage | tableFlags;
	}

	g_page_table table = (g_page_table) (directory[ti] & 0xFFFFF000);
	if(table[pi])
		panic("pagingMapPage: duplicate mapping to address %h -> %h, table value: %h", virtualAddress, physicalAddress, table[pi]);

	table[pi] = physicalAddress | pageFlags;
	G_INVLPG(virtualAddress);
}

void pagingEnable()
{
	logDebug("%! enabling", "paging");

	uint32_t cr0;
	asm volatile("mov %%cr0, %0"
				 : "=b"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0, %%cr0" ::"b"(cr0));
}
