/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max SchlÃ¼ssel <lokoxe@gmail.com>                     *
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

#include "kernel/tasking/elf32_loader.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "shared/logger/logger.hpp"

/**
 * Spawns a ramdisk file as a process.
 */
g_elf32_spawn_status elf32SpawnFromRamdisk(g_ramdisk_entry* entry, g_security_level securityLevel, g_task** target)
{

	// Check file
	if(entry == 0 || entry->type != G_RAMDISK_ENTRY_TYPE_FILE)
	{
		return ELF32_SPAWN_STATUS_FILE_NOT_FOUND;
	}

	// Get and validate ELF header
	elf32_ehdr* header = (elf32_ehdr*) entry->data;
	g_elf32_validation_status status = elf32Validate(header);

	if(status == ELF32_VALIDATION_SUCCESSFUL)
	{
		g_process* process = taskingCreateProcess();

		// Temporarily switch to process space
		g_physical_address returnDirectory = taskingTemporarySwitchToSpace(process->pageDirectory);

		// Load binary
		elf32LoadBinaryToCurrentAddressSpace(header, process, securityLevel);

		// Create task
		g_task* mainTask = taskingCreateThread(0, process, securityLevel);
		if(mainTask == 0)
		{
			logInfo("%! failed to create main thread to spawn ELF binary from ramdisk", "elf32");
			return ELF32_SPAWN_STATUS_PROCESS_CREATION_FAILED;
		}

		taskingPrepareThreadLocalStorage(mainTask);

		// Set the tasks entry point
		mainTask->state->eip = header->e_entry;

		taskingTemporarySwitchBack(returnDirectory);

		// Add to scheduling list
		taskingAssign(taskingGetLocal(), mainTask);

		// Set out parameter
		*target = mainTask;
		logDebug("%! loading binary: %s to task: %i", "elf32", entry->name, mainTask->id);
		return ELF32_SPAWN_STATUS_SUCCESSFUL;
	}

	return ELF32_SPAWN_STATUS_VALIDATION_ERROR;
}

/**
 * This method loads the binary with the given header to the current address space. Once finished,
 * the start and end address of the executable image in memory are written to the respective out parameters.
 */
void elf32LoadBinaryToCurrentAddressSpace(elf32_ehdr* header, g_process* process, g_security_level securityLevel)
{
	elf32LoadLoadSegment(header, process, securityLevel);
	elf32LoadTlsMasterCopy(header, process, securityLevel);
}

/**
 *
 */
void elf32LoadLoadSegment(elf32_ehdr* header, g_process* process, g_security_level securityLevel)
{
	// Flags for page tables/pages
	uint32_t tableFlags;
	uint32_t pageFlags;
	if(securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		tableFlags = DEFAULT_KERNEL_TABLE_FLAGS;
		pageFlags = DEFAULT_KERNEL_PAGE_FLAGS;
	} else
	{
		tableFlags = DEFAULT_USER_TABLE_FLAGS;
		pageFlags = DEFAULT_USER_PAGE_FLAGS;
	}

	// Initial values
	uint32_t imageStart = 0xFFFFFFFF;
	uint32_t imageEnd = 0;

	// First find out how much place the image needs in memory
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

	// Align the addresses
	imageStart = G_PAGE_ALIGN_DOWN(imageStart);
	imageEnd = G_PAGE_ALIGN_UP(imageEnd);

	// Map pages for the executable
	for(uint32_t virt = imageStart; virt < imageEnd; virt += G_PAGE_SIZE)
	{
		g_physical_address phys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pagingMapPage(virt, phys, tableFlags, pageFlags);
		pageReferenceTrackerIncrement(phys);
	}

	// Write the image to memory
	for(uint32_t i = 0; i < header->e_phnum; i++)
	{
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));
		if(programHeader->p_type != PT_LOAD)
			continue;
		memorySetBytes((void*) programHeader->p_vaddr, 0, programHeader->p_memsz);
		memoryCopy((void*) programHeader->p_vaddr, (uint8_t*) (((uint32_t) header) + programHeader->p_offset), programHeader->p_filesz);
	}

	// Set out parameters
	process->image.start = imageStart;
	process->image.end = imageEnd;
}

/**
 *
 */
void elf32LoadTlsMasterCopy(elf32_ehdr* header, g_process* process, g_security_level securityLevel)
{
	// Flags for page tables/pages
	uint32_t tableFlags;
	uint32_t pageFlags;
	if(securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		tableFlags = DEFAULT_KERNEL_TABLE_FLAGS;
		pageFlags = DEFAULT_KERNEL_PAGE_FLAGS;
	} else
	{
		tableFlags = DEFAULT_USER_TABLE_FLAGS;
		pageFlags = DEFAULT_USER_PAGE_FLAGS;
	}

	// Map pages for TLS master copy
	uint32_t tlsSize = 0;
	for(uint32_t i = 0; i < header->e_phnum; i++)
	{
		elf32_phdr* programHeader = (elf32_phdr*) (((uint32_t) header) + header->e_phoff + (header->e_phentsize * i));
		if(programHeader->p_type == PT_TLS)
		{
			tlsSize = G_PAGE_ALIGN_UP(programHeader->p_memsz);

			uint32_t tlsPages = tlsSize / G_PAGE_SIZE;
			uint32_t tlsStart = addressRangePoolAllocate(process->virtualRangePool, tlsPages);
			uint32_t tlsEnd = tlsStart + tlsSize;

			for(uint32_t virt = tlsStart; virt < tlsEnd; virt += G_PAGE_SIZE)
			{
				g_physical_address phys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
				pagingMapPage(virt, phys, tableFlags, pageFlags);
				pageReferenceTrackerIncrement(phys);
			}

			memorySetBytes((void*) tlsStart, 0, programHeader->p_memsz);
			memoryCopy((void*) tlsStart, (uint8_t*) (((uint32_t) header) + programHeader->p_offset), programHeader->p_filesz);
			break;
		}
	}

}

/**
 * Validates if the executable with the given header can be run on this system.
 */
g_elf32_validation_status elf32Validate(elf32_ehdr* header)
{

	// Valid ELF header
	if(/**/(header->e_ident[EI_MAG0] != ELFMAG0) || // 0x7F
			(header->e_ident[EI_MAG1] != ELFMAG1) || // E
			(header->e_ident[EI_MAG2] != ELFMAG2) || // L
			(header->e_ident[EI_MAG3] != ELFMAG3))   // F
	{
		return ELF32_VALIDATION_NOT_ELF;
	}

	// Must be executable
	if(header->e_type != ET_EXEC)
	{
		return ELF32_VALIDATION_NOT_EXECUTABLE;
	}

	// Must be i386 architecture compatible
	if(header->e_machine != EM_386)
	{
		return ELF32_VALIDATION_NOT_I386;
	}

	// Must be 32 bit
	if(header->e_ident[EI_CLASS] != ELFCLASS32)
	{
		return ELF32_VALIDATION_NOT_32BIT;
	}

	// Must be little endian
	if(header->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return ELF32_VALIDATION_NOT_LITTLE_ENDIAN;
	}

	// Must comply to current ELF standard
	if(header->e_version != EV_CURRENT)
	{
		return ELF32_VALIDATION_NOT_STANDARD_ELF;
	}

	// All fine
	return ELF32_VALIDATION_SUCCESSFUL;
}

