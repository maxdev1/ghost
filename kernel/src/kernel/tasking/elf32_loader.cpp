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
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/memory.hpp"
#include "shared/logger/logger.hpp"

#define MAXIMUM_LOAD_PAGES_AT_ONCE 0x10

g_spawn_status elf32Spawn(g_task* caller, g_fd fd, g_security_level securityLevel, g_task** outTask, g_spawn_validation_details* outValidationDetails)
{
	elf32_ehdr header;
	g_spawn_validation_details status = elf32ReadAndValidateHeader(caller, fd, &header);
	if(outValidationDetails)
		*outValidationDetails = status;
	if(status != G_SPAWN_VALIDATION_SUCCESSFUL)
	{
		return G_SPAWN_STATUS_FORMAT_ERROR;
	}

	// Create a new process & load binary
	g_process* targetProcess = taskingCreateProcess();
	g_spawn_status spawnStatus = elf32LoadBinaryToProcessSpace(caller, fd, &header, targetProcess, securityLevel);
	if(spawnStatus != G_SPAWN_STATUS_SUCCESSFUL)
	{
		logInfo("%! failed to load binary to current address space", "spawner");
		return spawnStatus;
	}

	// Create main task AFTER loading binary so it can copy it's TLS
	g_task* targetTask = taskingCreateThread(header.e_entry, targetProcess, securityLevel);
	if(targetTask == 0)
	{
		logInfo("%! failed to create main thread to spawn ELF binary from ramdisk", "elf32");
		return G_SPAWN_STATUS_TASKING_ERROR;
	}

	// Add to scheduling list
	taskingAssign(taskingGetLocal(), targetTask);

	// Set out parameter
	if(outTask)
		*outTask = targetTask;
	logDebug("%! loading binary: %s to task: %i", "elf32", entry->name, targetTask->id);
	return G_SPAWN_STATUS_SUCCESSFUL;
}

bool elf32ReadAllBytes(g_task* caller, g_fd fd, size_t offset, uint8_t* buffer, uint64_t len)
{
	int64_t seeked;
	g_fs_seek_status seekStatus = filesystemSeek(caller, fd, G_FS_SEEK_SET, offset, &seeked);
	if(seekStatus != G_FS_SEEK_SUCCESSFUL)
		return false;
	if(seeked != offset)
		logInfo("%! tried to seek in file to position %i but only got to %i", "spawner", (uint32_t )offset, (uint32_t )seeked);

	uint64_t remain = len;
	while(remain)
	{
		int64_t read;
		g_fs_read_status readStatus = filesystemRead(caller, fd, &buffer[len - remain], remain, &read);
		if(readStatus != G_FS_READ_SUCCESSFUL)
		{
			logInfo("%! failed to read binary from fd %i", "spawner", fd);
			return false;
		}
		remain -= read;
	}
	return true;
}

g_spawn_validation_details elf32ReadAndValidateHeader(g_task* caller, g_fd file, elf32_ehdr* headerBuffer)
{
	if(!elf32ReadAllBytes(caller, file, 0, (uint8_t*) headerBuffer, sizeof(elf32_ehdr)))
	{
		logInfo("%! failed to spawn file %i due to io error", "spawner", file);
		return G_SPAWN_VALIDATION_ELF32_IO_ERROR;
	}
	return elf32Validate(headerBuffer);
}

g_spawn_validation_details elf32Validate(elf32_ehdr* header)
{
	// Valid ELF header
	if(/**/(header->e_ident[EI_MAG0] != ELFMAG0) || // 0x7F
			(header->e_ident[EI_MAG1] != ELFMAG1) ||	  // E
			(header->e_ident[EI_MAG2] != ELFMAG2) ||	  // L
			(header->e_ident[EI_MAG3] != ELFMAG3))		  // F
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_ELF;
	}

	// Must be executable
	if(header->e_type != ET_EXEC)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_EXECUTABLE;
	}

	// Must be i386 architecture compatible
	if(header->e_machine != EM_386)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_I386;
	}

	// Must be 32 bit
	if(header->e_ident[EI_CLASS] != ELFCLASS32)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_32BIT;
	}

	// Must be little endian
	if(header->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_LITTLE_ENDIAN;
	}

	// Must comply to current ELF standard
	if(header->e_version != EV_CURRENT)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_STANDARD_ELF;
	}

	// All fine
	return G_SPAWN_VALIDATION_SUCCESSFUL;
}

g_spawn_status elf32LoadBinaryToProcessSpace(g_task* caller, g_fd file, elf32_ehdr* header, g_process* targetProcess, g_security_level securityLevel)
{
	g_physical_address returnDirectory = taskingTemporarySwitchToSpace(targetProcess->pageDirectory);
	g_spawn_status status = G_SPAWN_STATUS_SUCCESSFUL;

	for(uint32_t i = 0; i < header->e_phnum; i++)
	{
		uint32_t phdrOffset = header->e_phoff + header->e_phentsize * i;
		uint32_t phdrLength = sizeof(elf32_phdr);
		uint8_t phdrBuffer[phdrLength];

		if(!elf32ReadAllBytes(caller, file, phdrOffset, phdrBuffer, phdrLength))
		{
			logInfo("%! failed to read segment header from file %i", "spawner", file);
			status = G_SPAWN_STATUS_IO_ERROR;
			break;
		} else
		{
			elf32_phdr* phdr = (elf32_phdr*) phdrBuffer;

			if(phdr->p_type == PT_LOAD)
			{
				status = elf32LoadLoadSegment(caller, file, phdr, targetProcess);
				if(status != G_SPAWN_STATUS_SUCCESSFUL)
				{
					logInfo("%! unable to load PT_LOAD segment from file", "spawner");
					break;
				}
			} else if(phdr->p_type == PT_TLS)
			{
				status = elf32LoadTlsMasterCopy(caller, file, phdr, targetProcess);
				if(status != G_SPAWN_STATUS_SUCCESSFUL)
				{
					logInfo("%! unable to load PT_TLS segment from file", "spawner");
					break;
				}
			}
		}
	}

	taskingTemporarySwitchBack(returnDirectory);
	return status;
}

g_spawn_status elf32LoadLoadSegment(g_task* caller, g_fd file, elf32_phdr* phdr, g_process* targetProcess)
{
	uint32_t imageStart = phdr->p_vaddr & ~0xFFF;
	uint32_t imageEnd = ((phdr->p_vaddr + phdr->p_memsz) + 0x1000) & ~0xFFF;

	uint32_t pagesTotal = (imageEnd - imageStart) / 0x1000;
	uint32_t pagesLoaded = 0;

	uint32_t offsetInFile = 0;

	while(pagesLoaded < pagesTotal)
	{
		uint32_t startVirt = imageStart + pagesLoaded * G_PAGE_SIZE;

		// Map next chunk of memory
		uint32_t chunkPages = (pagesTotal - pagesLoaded);
		if(chunkPages > MAXIMUM_LOAD_PAGES_AT_ONCE)
		{
			chunkPages = MAXIMUM_LOAD_PAGES_AT_ONCE;
		}
		for(uint32_t i = 0; i < chunkPages; i++)
		{
			g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
			pageReferenceTrackerIncrement(page);
			pagingMapPage(startVirt + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		}
		uint8_t* area = (uint8_t*) startVirt;

		// Is there anything left to copy?
		uint32_t virtualFileEnd = phdr->p_vaddr + phdr->p_filesz;
		uint32_t bytesToCopy = 0;

		if(startVirt < virtualFileEnd)
		{
			// Check if file ends in this area
			if((virtualFileEnd >= startVirt) && (virtualFileEnd < (startVirt + chunkPages * G_PAGE_SIZE)))
			{
				bytesToCopy = virtualFileEnd - startVirt;
			} else
			{
				bytesToCopy = chunkPages * G_PAGE_SIZE;
			}
		}

		// Read file to memory
		if(!elf32ReadAllBytes(caller, file, phdr->p_offset + offsetInFile, area, bytesToCopy))
		{
			logInfo("%! unable to read LOAD segment", "spawner");
			return G_SPAWN_STATUS_IO_ERROR;
		}

		// Zero area before content in memory (if we are at the start)
		if(imageStart == startVirt && phdr->p_vaddr - startVirt > 0)
		{
			memorySetBytes(area, 0, startVirt);
		}

		// Zero memory after content in memory
		if(bytesToCopy < chunkPages * G_PAGE_SIZE)
		{
			uint32_t endZeroAreaStart = ((uint32_t) area) + bytesToCopy;
			uint32_t endZeroAreaLength = (chunkPages * G_PAGE_SIZE) - bytesToCopy;
			memorySetBytes((void*) endZeroAreaStart, 0, endZeroAreaLength);
		}

		pagesLoaded += chunkPages;
		offsetInFile += bytesToCopy;
	}

	targetProcess->image.start = imageStart;
	targetProcess->image.end = imageEnd;
	return G_SPAWN_STATUS_SUCCESSFUL;
}

g_spawn_status elf32LoadTlsMasterCopy(g_task* caller, g_fd file, elf32_phdr* phdr, g_process* targetProcess)
{
	uint32_t bytesToCopy = phdr->p_filesz;
	uint32_t bytesToZero = phdr->p_memsz;

	uint8_t* tlsContentBuffer = (uint8_t*) heapAllocate(bytesToCopy);

	g_spawn_status status;
	if(!elf32ReadAllBytes(caller, file, phdr->p_offset, (uint8_t*) tlsContentBuffer, bytesToCopy))
	{
		status = G_SPAWN_STATUS_IO_ERROR;
		logInfo("%! unable to read TLS segment from file", "spawner");
	} else
	{
		uint32_t requiredPages = G_PAGE_ALIGN_UP(bytesToZero) / G_PAGE_SIZE;
		g_virtual_address tlsStart = addressRangePoolAllocate(targetProcess->virtualRangePool, requiredPages, G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER);

		for(uint32_t i = 0; i < requiredPages; i++)
		{
			g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
			pagingMapPage(tlsStart + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			pageReferenceTrackerIncrement(page);
		}

		memorySetBytes((uint8_t*) tlsStart, 0, bytesToZero);
		memoryCopy((uint8_t*) tlsStart, tlsContentBuffer, bytesToCopy);

		logDebug("%! initialized TLS with size of %i pages for target process %i", "spawner", requiredPages, targetProcess->id);

		targetProcess->tlsMaster.location = tlsStart;
		targetProcess->tlsMaster.alignment = phdr->p_align;
		targetProcess->tlsMaster.copysize = phdr->p_filesz;
		targetProcess->tlsMaster.totalsize = phdr->p_memsz;
		status = G_SPAWN_STATUS_SUCCESSFUL;
	}

	heapFree(tlsContentBuffer);
	return status;
}
