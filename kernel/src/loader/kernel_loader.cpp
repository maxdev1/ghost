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

#include "ghost/types.h"

#include "loader/kernel_loader.hpp"
#include "loader/loader.hpp"
#include "loader/memory/paging.hpp"

#include "shared/logger/logger.hpp"
#include "shared/memory/paging.hpp"
#include "shared/memory/memory.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"
#include "shared/video/pretty_boot.hpp"

/**
 * This must stay as a global variable. We can't put this on
 * the stack because we change ESP before calling it.
 */
static void (*kernelMain)(g_setup_information*);

void kernelLoaderLoad(g_multiboot_module* kernelModule)
{
	elf32_ehdr* elfHeader = (elf32_ehdr*) kernelModule->moduleStart;
	kernelLoaderCheckHeader(elfHeader);
	kernelLoaderLoadBinary(elfHeader);

	// Initialize rest of kernel space
	uint32_t* directory = (uint32_t*) 0xFFFFF000;
	for(uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(loaderSetupInformation.kernelImageStart); ti < 1024; ti++)
	{
		if(directory[ti] == 0)
		{
			uint32_t tableChunkAddress = (uint32_t) bitmapPageAllocatorAllocate(&loaderPhysicalAllocator);
			directory[ti] = tableChunkAddress | DEFAULT_KERNEL_TABLE_FLAGS;

			uint32_t* table = ((uint32_t*) 0xFFC00000) + (0x400 * ti);
			for(uint32_t i = 0; i < 1024; i++)
				table[i] = 0;
		}
	}

	// Start kernel stack after kernel image
	loaderSetupInformation.stackStart = loaderSetupInformation.kernelImageEnd;
	loaderSetupInformation.stackEnd = loaderSetupInformation.stackStart + G_PAGE_SIZE;

	g_virtual_address stackVirt = loaderSetupInformation.stackEnd - sizeof(uint32_t);
	g_physical_address stackPhys = bitmapPageAllocatorAllocate(&loaderPhysicalAllocator);
	if(stackPhys == 0)
	{
		G_PRETTY_BOOT_FAIL("Failed to allocate kernel stack");
		loaderPanic("%! out of pages when trying to create kernel stack", "kernload");
	}

	pagingMapPageToRecursiveDirectory(loaderSetupInformation.stackStart, stackPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);

	G_PRETTY_BOOT_STATUS("Starting system services", 10);
	kernelLoaderCreateHeap();
	kernelLoaderEnterMain(elfHeader->e_entry, stackVirt);
}

void kernelLoaderCreateHeap()
{
	uint32_t heapStart = loaderSetupInformation.stackEnd;
	uint32_t heapEnd = heapStart + G_KERNEL_HEAP_SIZE;
	for(uint32_t virt = heapStart; virt < heapEnd; virt += G_PAGE_SIZE)
	{
		uint32_t phys = bitmapPageAllocatorAllocate(&loaderPhysicalAllocator);
		if(phys == 0)
		{
			G_PRETTY_BOOT_FAIL("Failed to allocate kernel heap");
			loaderPanic("%! out of pages when trying to allocate kernel heap, allocated to %h", "kernload", virt);
		}

		pagingMapPageToRecursiveDirectory(virt, phys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}
	loaderSetupInformation.heapStart = heapStart;
	loaderSetupInformation.heapEnd = heapEnd;
}

void kernelLoaderEnterMain(g_address entryAddress, g_address kernelEsp)
{
	kernelMain = (void (*)(g_setup_information*)) entryAddress;

	// Switch to kernel stack & enter kernel
	asm volatile("mov %0, %%esp\n"
		"mov %%esp, %%ebp\n"
		:: "r"(kernelEsp));
	kernelMain(&loaderSetupInformation);
	// Will never reach here, so no need to clean up the stack
	loaderPanic("%! dropped out of kernel main, this should never happen", "kernload");
}

void kernelLoaderLoadBinary(elf32_ehdr* header)
{
	logDebug("%! loading binary to higher memory", "kernload");
	uint32_t imageStart = UINT32_MAX;
	uint32_t imageEnd = 0;

	for(uint32_t i = 0; i < header->e_phnum; i++)
	{
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));

		if(programHeader->p_type != PT_LOAD)
			continue;

		if(programHeader->p_vaddr < imageStart)
			imageStart = programHeader->p_vaddr;

		if(programHeader->p_vaddr + programHeader->p_memsz > imageEnd)
			imageEnd = programHeader->p_vaddr + programHeader->p_memsz;
	}

	imageStart = G_PAGE_ALIGN_DOWN(imageStart);
	imageEnd = G_PAGE_ALIGN_UP(imageEnd);
	logDebug("%! image spans from %h to %h", "kernload", imageStart, imageEnd);

	for(uint32_t virt = imageStart; virt < imageEnd; virt += G_PAGE_SIZE)
	{
		uint32_t phys = bitmapPageAllocatorAllocate(&loaderPhysicalAllocator);
		pagingMapPageToRecursiveDirectory(virt, phys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}

	for(uint32_t i = 0; i < header->e_phnum; i++)
	{
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));
		if(programHeader->p_type != PT_LOAD)
			continue;

		memorySetBytes((void*) programHeader->p_vaddr, 0, programHeader->p_memsz);
		memoryCopy((void*) programHeader->p_vaddr, (uint8_t*) (((uint32_t) header) + programHeader->p_offset), programHeader->p_filesz);
	}

	logDebug("%! kernel image loaded to space %h - %h", "kernload", imageStart, imageEnd);
	loaderSetupInformation.kernelImageStart = imageStart;
	loaderSetupInformation.kernelImageEnd = imageEnd;
}

void kernelLoaderCheckHeader(elf32_ehdr* header)
{

	if(!(header->e_ident[EI_MAG0] == ELFMAG0 && header->e_ident[EI_MAG1] == ELFMAG1 && header->e_ident[EI_MAG2] == ELFMAG2
			&& header->e_ident[EI_MAG3] == ELFMAG3))
		loaderPanic("%! binary is not ELF", "kernload");

	if(header->e_type != ET_EXEC)
		loaderPanic("%! binary is not executable", "kernload");

	if(header->e_machine != EM_386)
		loaderPanic("%! binary target architecture not i386", "kernload");

	if(header->e_ident[EI_CLASS] != ELFCLASS32)
		loaderPanic("%! binary is not 32bit", "kernload");

	if(header->e_ident[EI_DATA] != ELFDATA2LSB)
		loaderPanic("%! binary does not have little endian data encoding", "kernload");

	if(header->e_version != EV_CURRENT)
		loaderPanic("%! binary is not standard ELF", "kernload");

	logDebug("%! binary ELF validation successful", "kernload");
}
