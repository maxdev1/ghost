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
#include "kernel/calls/syscall.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/elf/elf_tls.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "shared/utils/string.hpp"

g_load_executable_result elfLoadExecutable(g_task* caller, g_fd fd, g_security_level securityLevel, g_process* process)
{
	g_load_executable_result res;

	auto rootRes = elfObjectLoad(caller, nullptr, "root", fd, 0);
	res.status = rootRes.status;
	res.validationDetails = rootRes.validation;

	g_address imageEnd = rootRes.nextFreeBase;
	if(rootRes.status == G_SPAWN_STATUS_SUCCESSFUL)
	{
		elfTlsCreateMasterImage(caller, fd, process, rootRes.object);
		imageEnd = elfUserProcessCreateInfo(process, rootRes.object, imageEnd, securityLevel);

		process->object = rootRes.object;
		process->image.start = rootRes.object->startAddress;
		process->image.end = imageEnd;

		res.entry = rootRes.object->header.e_entry;
	}

	return res;
}

g_virtual_address elfUserProcessCreateInfo(g_process* process, g_elf_object* executableObject, g_virtual_address executableImageEnd, g_security_level securityLevel)
{
	// Calculate required space
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

	// Map required memory all loaded objects
	uint32_t areaStart = executableImageEnd;
	uint32_t pages = G_PAGE_ALIGN_UP(totalRequired) / G_PAGE_SIZE;
	for(uint32_t i = 0; i < pages; i++)
	{
		g_physical_address page = memoryPhysicalAllocate();
		pagingMapPage(areaStart + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}

	// Fill with data
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

	info->syscallKernelEntry = syscall;
	process->userProcessInfo = info;

	return executableImageEnd + G_PAGE_ALIGN_UP(totalRequired);
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
		logInfo("%! tried to seek in file to position %i but only got to %i", "elf", (uint32_t) offset, (uint32_t) seeked);
		return false;
	}

	uint64_t remain = len;
	while(remain)
	{
		int64_t read;
		if(filesystemRead(caller, fd, &buffer[len - remain], remain, &read) != G_FS_READ_SUCCESSFUL ||
		   read == 0)
		{
			logInfo("%! failed to read binary from fd %i", "elf", fd);
			return false;
		}
		remain -= read;
	}
	return true;
}

g_spawn_validation_details elfReadAndValidateHeader(g_task* caller, g_fd file, Elf32_Ehdr* headerBuffer, bool root)
{
	if(!elfReadToMemory(caller, file, 0, (uint8_t*) headerBuffer, sizeof(Elf32_Ehdr)))
	{
		logInfo("%! failed to spawn file %i due to io error", "elf", file);
		return G_SPAWN_VALIDATION_ELF32_IO_ERROR;
	}
	return elfValidateHeader(headerBuffer, root);
}

g_spawn_validation_details elfValidateHeader(Elf32_Ehdr* header, bool root)
{
	if((header->e_ident[EI_MAG0] != ELFMAG0) || // 0x7F
	   (header->e_ident[EI_MAG1] != ELFMAG1) || // E
	   (header->e_ident[EI_MAG2] != ELFMAG2) || // L
	   (header->e_ident[EI_MAG3] != ELFMAG3))	// F
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_ELF;
	}

	// Check executable flag
	if(root && header->e_type != ET_EXEC)
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

	return G_SPAWN_VALIDATION_SUCCESSFUL;
}
