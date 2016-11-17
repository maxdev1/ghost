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

#include <executable/elf32_loader.hpp>

#include <kernel.hpp>
#include <logger/logger.hpp>
#include <ramdisk/ramdisk.hpp>
#include <tasking/tasking.hpp>
#include <tasking/thread_manager.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/physical/pp_reference_tracker.hpp>
#include <memory/paging.hpp>
#include <memory/address_space.hpp>
#include <memory/memory.hpp>
#include <memory/temporary_paging_util.hpp>
#include <memory/constants.hpp>
#include <utils/string.hpp>

/**
 * Spawns a ramdisk file as a process.
 */
g_elf32_spawn_status g_elf32_loader::spawnFromRamdisk(g_ramdisk_entry* entry, g_security_level securityLevel, g_thread** target, bool enforceCurrentCore,
		g_thread_priority priority) {

	// Check file
	if (entry == 0 || entry->type != G_RAMDISK_ENTRY_TYPE_FILE) {
		return ELF32_SPAWN_STATUS_FILE_NOT_FOUND;
	}

	// Get and validate ELF header
	elf32_ehdr* header = (elf32_ehdr*) entry->data;
	g_elf32_validation_status status = validate(header);

	if (status == ELF32_VALIDATION_SUCCESSFUL) {

		// Create the process
		g_thread* mainThread = g_thread_manager::createProcess(securityLevel, nullptr);
		if (mainThread == 0) {
			g_log_warn("%! failed to create main thread to spawn ELF binary from ramdisk", "elf32");
			return ELF32_SPAWN_STATUS_PROCESS_CREATION_FAILED;
		}
		g_process* process = mainThread->process;

		// Temporarily switch to the new processes page directory to load the binary to it
		g_page_directory thisPageDirectory = g_address_space::get_current_space();

		g_address_space::switch_to_space(process->pageDirectory);
		loadBinaryToCurrentAddressSpace(header, process);
		g_thread_manager::prepareThreadLocalStorage(mainThread);
		g_address_space::switch_to_space(thisPageDirectory);

		// Set the tasks entry point
		mainThread->cpuState->eip = header->e_entry;

		// Set priority
		mainThread->priority = priority;

		// Add to scheduling list
		g_tasking::addTask(mainThread, enforceCurrentCore);

		// Set out parameter
		*target = mainThread;
		return ELF32_SPAWN_STATUS_SUCCESSFUL;
	}

	return ELF32_SPAWN_STATUS_VALIDATION_ERROR;
}

/**
 * This method loads the binary with the given header to the current address space. Once finished,
 * the start and end address of the executable image in memory are written to the respective out parameters.
 */
void g_elf32_loader::loadBinaryToCurrentAddressSpace(elf32_ehdr* header, g_process* process) {

	loadLoadSegment(header, process);
	loadTlsMasterCopy(header, process);
}

/**
 *
 */
void g_elf32_loader::loadLoadSegment(elf32_ehdr* header, g_process* process) {

	// Initial values
	uint32_t imageStart = 0xFFFFFFFF;
	uint32_t imageEnd = 0;

	// First find out how much place the image needs in memory
	for (uint32_t i = 0; i < header->e_phnum; i++) {
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));
		if (programHeader->p_type != PT_LOAD)
			continue;
		if (programHeader->p_vaddr < imageStart)
			imageStart = programHeader->p_vaddr;
		if (programHeader->p_vaddr + programHeader->p_memsz > imageEnd)
			imageEnd = programHeader->p_vaddr + programHeader->p_memsz;
	}

	// Align the addresses
	imageStart = G_PAGE_ALIGN_DOWN(imageStart);
	imageEnd = G_PAGE_ALIGN_UP(imageEnd);

	// Map pages for the executable
	for (uint32_t virt = imageStart; virt < imageEnd; virt += G_PAGE_SIZE) {
		uint32_t phys = g_pp_allocator::allocate();
		g_address_space::map(virt, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		g_pp_reference_tracker::increment(phys);
	}

	// Write the image to memory
	for (uint32_t i = 0; i < header->e_phnum; i++) {
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));
		if (programHeader->p_type != PT_LOAD)
			continue;
		g_memory::setBytes((void*) programHeader->p_vaddr, 0, programHeader->p_memsz);
		g_memory::copy((void*) programHeader->p_vaddr, (uint8_t*) (((uint32_t) header) + programHeader->p_offset), programHeader->p_filesz);
	}

	// Set out parameters
	process->imageStart = imageStart;
	process->imageEnd = imageEnd;
}

/**
 *
 */
void g_elf32_loader::loadTlsMasterCopy(elf32_ehdr* header, g_process* process) {

	uint32_t tlsSize = 0;

	// Map pages for TLS master copy
	for (uint32_t i = 0; i < header->e_phnum; i++) {
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));
		if (programHeader->p_type == PT_TLS) {
			tlsSize = G_PAGE_ALIGN_UP(programHeader->p_memsz);

			uint32_t tlsPages = tlsSize / G_PAGE_SIZE;
			uint32_t tlsStart = process->virtualRanges.allocate(tlsPages);
			uint32_t tlsEnd = tlsStart + tlsSize;

			for (uint32_t virt = tlsStart; virt < tlsEnd; virt += G_PAGE_SIZE) {
				uint32_t phys = g_pp_allocator::allocate();
				g_address_space::map(virt, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
				g_pp_reference_tracker::increment(phys);
			}

			g_memory::setBytes((void*) tlsStart, 0, programHeader->p_memsz);
			g_memory::copy((void*) tlsStart, (uint8_t*) (((uint32_t) header) + programHeader->p_offset), programHeader->p_filesz);
			break;
		}
	}

}

/**
 * Validates if the executable with the given header can be run on this system.
 */
g_elf32_validation_status g_elf32_loader::validate(elf32_ehdr* header) {

	// Valid ELF header
	if (/**/(header->e_ident[EI_MAG0] != ELFMAG0) || // 0x7F
			(header->e_ident[EI_MAG1] != ELFMAG1) || // E
			(header->e_ident[EI_MAG2] != ELFMAG2) || // L
			(header->e_ident[EI_MAG3] != ELFMAG3))   // F
			{
		return ELF32_VALIDATION_NOT_ELF;
	}

	// Must be executable
	if (header->e_type != ET_EXEC) {
		return ELF32_VALIDATION_NOT_EXECUTABLE;
	}

	// Must be i386 architecture compatible
	if (header->e_machine != EM_386) {
		return ELF32_VALIDATION_NOT_I386;
	}

	// Must be 32 bit
	if (header->e_ident[EI_CLASS] != ELFCLASS32) {
		return ELF32_VALIDATION_NOT_32BIT;
	}

	// Must be little endian
	if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
		return ELF32_VALIDATION_NOT_LITTLE_ENDIAN;
	}

	// Must comply to current ELF standard
	if (header->e_version != EV_CURRENT) {
		return ELF32_VALIDATION_NOT_STANDARD_ELF;
	}

	// All fine
	return ELF32_VALIDATION_SUCCESSFUL;
}
