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
#include "loader/loader.hpp"

#include "shared/memory/paging.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"
#include "shared/logger/logger.hpp"

void pagingInitialize(g_virtual_address reservedAreaEnd)
{
	g_physical_address pageDirPhys = bitmapPageAllocatorAllocate(&loaderPhysicalAllocator);
	if(!pageDirPhys)
		loaderPanic("pagingInitialize: failed to allocate memory for page directory");

	loaderSetupInformation.initialPageDirectoryPhysical = pageDirPhys;
	g_page_directory pageDirectory = (g_page_directory) pageDirPhys;

	for(uint32_t i = 0; i < 1024; i++)
		pageDirectory[i] = 0;
	pageDirectory[1023] = pageDirPhys | DEFAULT_KERNEL_TABLE_FLAGS;

	// Identity-map reserved area;
	// User code must be able to access the lower memory, for example to access
	// memory blocks allocated in lower memory for VM86
	pagingIdentityMap(pageDirectory, 0, reservedAreaEnd, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	pagingRelocateMultibootModules(pageDirectory, reservedAreaEnd);
	pagingEnableGlobalPageFlag();
	pagingSwitchToSpace(pageDirPhys);
	pagingEnable();
}

void pagingEnableGlobalPageFlag()
{
	uint32_t cr4;
	asm volatile("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= (1 << 7);
	asm volatile("mov %0, %%cr4" :: "b"(cr4));
}

void pagingRelocateMultibootModules(g_page_directory pageDirectory, g_address startAt)
{
	g_multiboot_information* mbInfo = loaderSetupInformation.multibootInformation;
	g_address nextModuleLocation = startAt;

	for(int i = 0; i < (int) mbInfo->modulesCount; i++)
	{
		g_multiboot_module* module = (g_multiboot_module*) (mbInfo->modulesAddress + sizeof(g_multiboot_module) * i);
		uint32_t modPhysStartAligned = G_PAGE_ALIGN_DOWN(module->moduleStart);
		uint32_t modPhysOff = module->moduleStart - modPhysStartAligned;
		uint32_t modPhysEndAligned = G_PAGE_ALIGN_UP(module->moduleEnd);
		uint32_t modPhysLen = module->moduleEnd - module->moduleStart;

		uint32_t modVirtStartAligned = nextModuleLocation;
		uint32_t modVirtStart = modVirtStartAligned + modPhysOff;

		for(uint32_t off = 0; off < modPhysEndAligned - modPhysStartAligned; off += G_PAGE_SIZE)
			pagingMapPage(pageDirectory, modVirtStartAligned + off, modPhysStartAligned + off, DEFAULT_USER_TABLE_FLAGS,
			DEFAULT_USER_PAGE_FLAGS);

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
		pagingMapPage(directory, start, start, tableFlags, pageFlags);
		start += G_PAGE_SIZE;
	}
}

void pagingMapPage(g_page_directory directory, g_address virtualAddress, g_address physicalAddress, uint32_t tableFlags, uint32_t pageFlags)
{
	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virtualAddress);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virtualAddress);

	if(directory[ti] == 0)
	{
		g_physical_address tablePage = (uint32_t) bitmapPageAllocatorAllocate(&loaderPhysicalAllocator);
		if(!tablePage)
			loaderPanic("pagingMapPage: failed to map %h -> %h, could not allocate page for table", virtualAddress, physicalAddress);

		g_page_table table = (g_page_table) tablePage;
		for(uint32_t i = 0; i < 1024; i++)
			table[i] = 0;

		directory[ti] = tablePage | tableFlags;
	}

	g_page_table table = (g_page_table) (directory[ti] & 0xFFFFF000);
	if(table[pi])
		loaderPanic("pagingMapPage: duplicate mapping to address %h -> %h, table value: %h", virtualAddress, physicalAddress, table[pi]);

	table[pi] = physicalAddress | pageFlags;
	G_INVLPG(virtualAddress);
}

bool pagingMapPageToRecursiveDirectory(uint32_t virtualAddress, uint32_t physicalAddress, uint32_t tableFlags, uint32_t pageFlags)
{

	if((virtualAddress & G_PAGE_ALIGN_MASK) || (physicalAddress & G_PAGE_ALIGN_MASK))
	{
		loaderPanic("%! tried to map unaligned addresses: virt %h to phys %h", "paging", virtualAddress, physicalAddress);
	}

	uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(virtualAddress);
	uint32_t pi = G_PAGE_IN_TABLE_INDEX(virtualAddress);

	g_page_directory directory = (g_page_directory) 0xFFFFF000;
	g_page_table table = ((g_page_table) 0xFFC00000) + (0x400 * ti);

	if(directory[ti] == 0)
	{
		uint32_t tableChunkAddress = (uint32_t) bitmapPageAllocatorAllocate(&loaderPhysicalAllocator);
		directory[ti] = tableChunkAddress | tableFlags;

		for(uint32_t i = 0; i < 1024; i++)
			table[i] = 0;
	}

	if(table[pi] == 0)
	{
		table[pi] = physicalAddress | pageFlags;
		G_INVLPG(virtualAddress);
		return true;
	} else
	{
		logDebug("%! tried to map area that was already mapped, virt %h -> phys %h, table contains %h", "paging", virtualAddress, physicalAddress, table[pi]);
	}
	return false;
}

void pagingEnable()
{
	logDebug("%! enabling", "paging");

	uint32_t cr0;
	asm volatile("mov %%cr0, %0": "=b"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0, %%cr0":: "b"(cr0));
}
