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

#include <kernelloader/kernel_loader.hpp>
#include <loader.hpp>
#include <logger/logger.hpp>
#include <memory/paging.hpp>
#include <memory/paging_initializer.hpp>
#include <memory/memory.hpp>
#include <memory/bitmap/bitmap_page_allocator.hpp>
#include <video/pretty_boot.hpp>

/**
 * 
 */
void g_kernel_loader::load(g_multiboot_module* kernelModule) {

	g_setup_information* setupInformation = g_loader::getSetupInformation();

	// ELF header
	elf32_ehdr* elfHeader = (elf32_ehdr*) kernelModule->moduleStart;
	checkHeader(elfHeader);

	// Load the binary
	loadBinary(elfHeader, setupInformation);

	/**
	 * Create all page tables for kernel area to initial directory. This is done to ensure
	 * that every copy of the kernel space contains all later kernel pages.
	 */
	uint32_t* directory = (uint32_t*) 0xFFFFF000;
	for (uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(setupInformation->kernelImageStart); ti < 1024; ti++) {

		// Create the table only if it doesn't exist yet
		if (directory[ti] == 0) {
			uint32_t tableChunkAddress = (uint32_t) g_loader::getPhysicalAllocator()->allocate();
			directory[ti] = tableChunkAddress | DEFAULT_KERNEL_TABLE_FLAGS;

			// Zero the new table
			uint32_t* table = ((uint32_t*) 0xFFC00000) + (0x400 * ti);
			for (uint32_t i = 0; i < 1024; i++) {
				table[i] = 0;
			}
		}
	}

	// Create new stack
	setupInformation->stackStart = setupInformation->kernelImageEnd;
	setupInformation->stackEnd = setupInformation->stackStart + G_PAGE_SIZE;
	uint32_t stackPointer = setupInformation->stackEnd - 4;
	uint32_t stackPhys = g_loader::getPhysicalAllocator()->allocate();
	if (stackPhys == 0) {
		G_PRETTY_BOOT_FAIL("Failed to allocate kernel stack");
		g_loader::panic("%! out of pages when trying to create kernel stack", "kernload");
	}
	g_paging_initializer::mapPageToRecursiveDirectory(setupInformation->stackStart, stackPhys, DEFAULT_KERNEL_TABLE_FLAGS,
	DEFAULT_KERNEL_PAGE_FLAGS);

	// Create kernel heap
	uint32_t heapStart = setupInformation->stackEnd;
	uint32_t heapEnd = heapStart + G_KERNEL_HEAP_SIZE;
	for (uint32_t virt = heapStart; virt < heapEnd; virt += G_PAGE_SIZE) {
		uint32_t phys = g_loader::getPhysicalAllocator()->allocate();

		if (phys == 0) {
			G_PRETTY_BOOT_FAIL("Failed to allocate kernel heap");
			g_loader::panic("%! out of pages when trying to allocate kernel heap, allocated to %h", "kernload", virt);
		}

		g_paging_initializer::mapPageToRecursiveDirectory(virt, phys,
		DEFAULT_KERNEL_TABLE_FLAGS,
		DEFAULT_KERNEL_PAGE_FLAGS);
	}
	setupInformation->heapStart = heapStart;
	setupInformation->heapEnd = heapEnd;

	// Push information and enter kernel
	uint32_t setupInformationAddress = (uint32_t) g_loader::getSetupInformation();
	uint32_t entryAddress = elfHeader->e_entry;

	G_PRETTY_BOOT_STATUS("Starting kernel", 10);
	g_log_info("%! jumping to %h...", "kernload", entryAddress);
	asm("mov %0, %%esp\n"
			"mov %%esp, %%ebp\n"
			"push %1\n"
			"call *%2" :: "r"(stackPointer), "r"(setupInformationAddress), "r"(entryAddress));
	// Will never reach here, so no need to clean up the stack.
}

/**
 * 
 */
void g_kernel_loader::loadBinary(elf32_ehdr* header, g_setup_information* setupInformation) {

	g_log_debug("%! loading binary to higher memory", "kernload");
	uint32_t imageStart = -1;
	uint32_t imageEnd = 0;

	// Calculate image start and end
	for (uint32_t i = 0; i < header->e_phnum; i++) {
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));

		if (programHeader->p_type != PT_LOAD) {
			continue;
		}

		if (programHeader->p_vaddr < imageStart) {
			imageStart = programHeader->p_vaddr;
		}

		if (programHeader->p_vaddr + programHeader->p_memsz > imageEnd) {
			imageEnd = programHeader->p_vaddr + programHeader->p_memsz;
		}
	}

	imageStart = G_PAGE_ALIGN_DOWN(imageStart);
	imageEnd = G_PAGE_ALIGN_UP(imageEnd);
	g_log_debug("%! image spans from %h to %h", "kernload", imageStart, imageEnd);

	// Map pages
	for (uint32_t virt = imageStart; virt < imageEnd; virt += G_PAGE_SIZE) {
		uint32_t phys = g_loader::getPhysicalAllocator()->allocate();

		g_paging_initializer::mapPageToRecursiveDirectory(virt, phys,
		DEFAULT_KERNEL_TABLE_FLAGS,
		DEFAULT_KERNEL_PAGE_FLAGS);
	}

	// Copy image
	for (uint32_t i = 0; i < header->e_phnum; i++) {
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));

		if (programHeader->p_type != PT_LOAD) {
			continue;
		}

		// Clear
		g_memory::setBytes((void*) programHeader->p_vaddr, 0, programHeader->p_memsz);

		// Copy contents
		g_memory::copy((void*) programHeader->p_vaddr, (uint8_t*) (((uint32_t) header) + programHeader->p_offset), programHeader->p_filesz);
	}

	g_log_debug("%! kernel image loaded to space %h - %h", "kernload", imageStart, imageEnd);
	setupInformation->kernelImageStart = imageStart;
	setupInformation->kernelImageEnd = imageEnd;
}

/**
 * 
 */
void g_kernel_loader::checkHeader(elf32_ehdr* header) {

	if (!(header->e_ident[EI_MAG0] == ELFMAG0 && header->e_ident[EI_MAG1] == ELFMAG1 && header->e_ident[EI_MAG2] == ELFMAG2
			&& header->e_ident[EI_MAG3] == ELFMAG3)) {
		g_loader::panic("%! binary is not ELF", "kernload");
	}

	if (header->e_type != ET_EXEC) {
		g_loader::panic("%! binary is not executable", "kernload");
	}

	if (header->e_machine != EM_386) {
		g_loader::panic("%! binary target architecture not i386", "kernload");
	}

	if (header->e_ident[EI_CLASS] != ELFCLASS32) {
		g_loader::panic("%! binary is not 32bit", "kernload");
	}

	if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
		g_loader::panic("%! binary does not have little endian data encoding", "kernload");
	}

	if (header->e_version != EV_CURRENT) {
		g_loader::panic("%! binary is not standard ELF", "kernload");
	}

	g_log_debug("%! binary ELF validation successful", "kernload");
}
