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

#include "kernel/tasking/elf/elf_loader.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/memory.hpp"
#include "shared/logger/logger.hpp"


g_spawn_status elfLoadExecutable(g_task* caller, g_fd fd, g_security_level securityLevel, g_process** outProcess, g_spawn_validation_details* outDetails)
{
	/* Create process and load binary */
	g_process* targetProcess = taskingCreateProcess();
	g_physical_address returnDirectory = taskingTemporarySwitchToSpace(targetProcess->pageDirectory);

	g_elf_object* executableObject;
	g_virtual_address executableImageEnd;
	g_spawn_validation_details validationDetails;
	g_spawn_status spawnStatus = elfObjectLoad(caller, 0, "main", fd, 0, targetProcess->virtualRangePool, &executableImageEnd, &executableObject, &validationDetails);
	elf32TlsCreateMasterImage(caller, fd, targetProcess, executableObject);
	executableImageEnd = elfUserProcessCreateInfo(targetProcess, executableObject, executableImageEnd);

	taskingTemporarySwitchBack(returnDirectory);

	/* Cancel if validation has failed */
	if(outDetails) *outDetails = validationDetails;
	if(spawnStatus != G_SPAWN_STATUS_SUCCESSFUL)
	{
		logInfo("%! failed to load binary to current address space", "elf");
		return spawnStatus;
	}

	/* Update process */	
	targetProcess->image.start = executableObject->startAddress;
	targetProcess->image.end = executableImageEnd;
	logDebug("%! process loaded to %h - %h", "elf", targetProcess->image.start, targetProcess->image.end);

	/* Create main thread */
	g_task* thread = taskingCreateThread(executableObject->header.e_entry, targetProcess, securityLevel);
	if(thread == 0)
	{
		logInfo("%! failed to create main thread to spawn ELF binary from ramdisk", "elf");
		return G_SPAWN_STATUS_TASKING_ERROR;
	}
	taskingAssign(taskingGetLocal(), thread);

	if(outProcess) *outProcess = targetProcess;
	return G_SPAWN_STATUS_SUCCESSFUL;
}

g_virtual_address elfUserProcessCreateInfo(g_process* process, g_elf_object* executableObject, g_virtual_address executableImageEnd)
{
	/* Calculate required space */
	int objectCount = 0;
	uint32_t stringTableSize = 0;

	auto it = hashmapIteratorStart(executableObject->loadedObjects);
	while(hashmapIteratorHasNext(&it))
	{
		auto object = hashmapIteratorNext(&it)->value;
		stringTableSize += stringLength(object->name) + 1;
		objectCount++;
	}
	hashmapIteratorEnd(&it);
	uint32_t totalRequired = sizeof(g_process_info) + sizeof(g_object_info) * objectCount + stringTableSize;

	/* Preserve memory after executable */
	uint32_t areaStart = executableImageEnd;
	uint32_t pages = G_PAGE_ALIGN_UP(totalRequired) / G_PAGE_SIZE;
	for(uint32_t i = 0; i < pages; i++)
	{
		g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pageReferenceTrackerIncrement(page);
		pagingMapPage(areaStart + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}

	/* Fill with data */
	g_process_info* info = (g_process_info*) areaStart;
	g_object_info* objectInfo = (g_object_info*) (areaStart + sizeof(g_process_info));
	char* stringTable = (char*) ((g_virtual_address) objectInfo + (sizeof(g_object_info) * objectCount));

	memorySetBytes((void*) info, 0, sizeof(g_process_info));
	info->objectInfosSize = objectCount;
	info->objectInfos = objectInfo;

	it = hashmapIteratorStart(executableObject->loadedObjects);
	while(hashmapIteratorHasNext(&it))
	{
		g_elf_object* object = hashmapIteratorNext(&it)->value;

		memorySetBytes((void*) objectInfo, 0, sizeof(g_object_info));

		objectInfo->name = stringTable;
		stringCopy(stringTable, object->name);
		stringTable += stringLength(object->name) + 1;

		objectInfo->preinitArray = object->preinitArray;
		objectInfo->preinitArraySize = object->preinitArraySize;
		objectInfo->init = object->init;
		objectInfo->initArray = object->initArray;
		objectInfo->initArraySize = object->initArraySize;
		objectInfo->fini = object->fini;
		objectInfo->finiArray = object->finiArray;
		objectInfo->finiArraySize = object->finiArraySize;

		objectInfo++;
	}
	hashmapIteratorEnd(&it);

	process->userProcessInfo = info;

	return executableImageEnd + G_PAGE_ALIGN_UP(totalRequired);
}

g_spawn_status elfLoadLoadSegment(g_task* caller, g_fd file, elf32_phdr* phdr, g_virtual_address baseAddress, g_elf_object* object)
{
	/* Calculate addresses where segment is loaded */
	g_virtual_address loadBase = baseAddress + phdr->p_vaddr;
	g_virtual_address loadEnd = loadBase + phdr->p_filesz;
	g_virtual_address memoryStart = loadBase & ~G_PAGE_ALIGN_MASK;
	g_virtual_address memoryEnd = ((loadBase + phdr->p_memsz) & ~G_PAGE_ALIGN_MASK) + G_PAGE_SIZE;

	if(object->startAddress == 0 || memoryStart < object->startAddress) {
		object->startAddress = memoryStart;
	}
	if(memoryEnd > object->endAddress) {
		object->endAddress = memoryEnd;
	}

	uint32_t pagesTotal = (memoryEnd - memoryStart) / G_PAGE_SIZE;
	uint32_t pagesLoaded = 0;

	uint32_t loadPosition = loadBase;
	uint32_t readOffset = phdr->p_offset;
	while(pagesLoaded < pagesTotal)
	{
		/* Allocate memory */
		uint32_t areaPages = (pagesTotal - pagesLoaded);
		if(areaPages > ELF_MAXIMUM_LOAD_PAGES_AT_ONCE)
			areaPages = ELF_MAXIMUM_LOAD_PAGES_AT_ONCE;

		g_virtual_address areaStart = memoryStart + pagesLoaded * G_PAGE_SIZE;
		g_virtual_address areaEnd = (areaStart + areaPages * G_PAGE_SIZE);

		for(uint32_t i = 0; i < areaPages; i++)
		{
			g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
			pageReferenceTrackerIncrement(page);
			pagingMapPage(areaStart + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		}

		if(loadPosition < loadEnd)
		{
			uint32_t copyBytes = 0;
			if(loadEnd < areaEnd) {
				copyBytes = loadEnd - loadPosition;
			} else {
				copyBytes = areaEnd - loadPosition;
			}

			/* Zero area before content in memory (if we are at the start) */
			uint32_t sizeBefore = loadPosition - areaStart;
			if(sizeBefore > 0)
			{
				memorySetBytes((void*) areaStart, 0, sizeBefore);

				logDebug("%!   [%h-%h] zero %h bytes before code", "elf", areaStart, areaStart + sizeBefore, sizeBefore);
			}

			/* Copy data to memory */
			if(!elfReadToMemory(caller, file, readOffset, (uint8_t*) loadPosition, copyBytes))
			{
				logInfo("%! unable to read data for PT_LOAD header", "elf");
				return G_SPAWN_STATUS_IO_ERROR;
			}
			logDebug("%!   [%h-%h] read %h bytes from file %h", "elf", loadPosition, loadPosition + copyBytes, copyBytes, readOffset);

			/* Zero memory after content in memory */
			uint32_t loadEnd = loadPosition + copyBytes;
			uint32_t sizeAfter = areaEnd - loadEnd;
			if(sizeAfter > 0)
			{
				memorySetBytes((void*) loadEnd, 0, sizeAfter);

				logDebug("%!   [%h-%h] zero %h bytes after code", "elf", loadEnd, loadEnd + sizeAfter, sizeAfter);
			}

			loadPosition += copyBytes;
			readOffset += copyBytes;

		} else {
			/* Zero area without any content */
			uint32_t areaSize = areaPages * G_PAGE_SIZE;
			memorySetBytes((void*) areaStart, 0, areaSize);
			loadPosition += areaSize;

			logDebug("%!   [%h-%h] zero %h bytes of blank", "elf", areaStart, areaEnd, areaSize);
		}

		pagesLoaded += areaPages;
	}

	return G_SPAWN_STATUS_SUCCESSFUL;
}

bool elfReadToMemory(g_task* caller, g_fd fd, size_t offset, uint8_t* buffer, uint64_t len)
{
	int64_t seeked;
	g_fs_seek_status seekStatus = filesystemSeek(caller, fd, G_FS_SEEK_SET, offset, &seeked);
	if(seekStatus != G_FS_SEEK_SUCCESSFUL)
	{
		logInfo("%! failed to seek to %i binary from fd %i", "elf", offset, fd);
		return false;
	}
	if(seeked != offset)
	{
		logInfo("%! tried to seek in file to position %i but only got to %i", "elf", (uint32_t )offset, (uint32_t )seeked);
	}

	uint64_t remain = len;
	while(remain)
	{
		int64_t read;
		g_fs_read_status readStatus = filesystemRead(caller, fd, &buffer[len - remain], remain, &read);
		if(readStatus != G_FS_READ_SUCCESSFUL)
		{
			logInfo("%! failed to read binary from fd %i", "elf", fd);
			return false;
		}
		remain -= read;
	}
	return true;
}

g_spawn_validation_details elfReadAndValidateHeader(g_task* caller, g_fd file, elf32_ehdr* headerBuffer, bool executable)
{
	if(!elfReadToMemory(caller, file, 0, (uint8_t*) headerBuffer, sizeof(elf32_ehdr)))
	{
		logInfo("%! failed to spawn file %i due to io error", "elf", file);
		return G_SPAWN_VALIDATION_ELF32_IO_ERROR;
	}
	return elfValidate(headerBuffer, executable);
}

g_spawn_validation_details elfValidate(elf32_ehdr* header, bool executable)
{
	if(/* */(header->e_ident[EI_MAG0] != ELFMAG0) ||      // 0x7F
			(header->e_ident[EI_MAG1] != ELFMAG1) ||	  // E
			(header->e_ident[EI_MAG2] != ELFMAG2) ||	  // L
			(header->e_ident[EI_MAG3] != ELFMAG3))		  // F
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_ELF;
	}

	/* Check executable flag */
	if(executable && header->e_type != ET_EXEC)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_EXECUTABLE;
	}

	/* Must be i386 architecture compatible */
	if(header->e_machine != EM_386)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_I386;
	}

	/* Must be 32 bit */
	if(header->e_ident[EI_CLASS] != ELFCLASS32)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_32BIT;
	}

	/* Must be little endian */
	if(header->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_LITTLE_ENDIAN;
	}

	/* Must comply to current ELF standard */
	if(header->e_version != EV_CURRENT)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_STANDARD_ELF;
	}

	return G_SPAWN_VALIDATION_SUCCESSFUL;
}
