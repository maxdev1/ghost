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

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/elf/elf_loader.hpp"

g_spawn_status elfTlsLoadData(g_fd file, Elf32_Phdr phdr, g_elf_object* object)
{
	uint32_t bytesToCopy = phdr.p_filesz;

	// Read TLS content to a buffer
	uint8_t* tlsContentBuffer = (uint8_t*) heapAllocate(bytesToCopy);
	if(!filesystemReadToMemory(file, phdr.p_offset, (uint8_t*) tlsContentBuffer, bytesToCopy))
	{
		logInfo("%! unable to read TLS segment from file", "elf");
		heapFree(tlsContentBuffer);
		return G_SPAWN_STATUS_IO_ERROR;
	}

	// Write object information
	object->tlsPart.content = tlsContentBuffer;
	object->tlsPart.alignment = phdr.p_align;
	object->tlsPart.copysize = phdr.p_filesz;
	object->tlsPart.totalsize = phdr.p_memsz;

	// Calculate offset in TLS master image
	if(object->root)
	{
		// Executable gets offset 0 and is followed with the g_user_threadlocal
		object->tlsPart.offset = 0;
		uint32_t size = G_ALIGN_UP(object->tlsPart.totalsize, object->tlsPart.alignment);
		object->tlsMaster.totalSize = size + sizeof(g_user_threadlocal);
		object->tlsMaster.userThreadOffset = size;
	}
	else
	{
		// Shared libraries just get the next free position
		g_elf_object* rootObject = object;
		while(rootObject->parent)
			rootObject = rootObject->parent;

		object->tlsPart.offset = rootObject->tlsMaster.totalSize;
		rootObject->tlsMaster.totalSize += G_ALIGN_UP(object->tlsPart.totalsize, object->tlsPart.alignment);
	}

	return G_SPAWN_STATUS_SUCCESSFUL;
}

void elfTlsCreateMasterImage(g_fd file, g_process* process, g_elf_object* rootObject)
{
	// Allocate memory
	uint32_t size = G_PAGE_ALIGN_UP(rootObject->tlsMaster.totalSize);
	uint32_t requiredPages = size / G_PAGE_SIZE;
	g_virtual_address tlsStart = addressRangePoolAllocate(process->virtualRangePool, requiredPages);
	for(uint32_t i = 0; i < requiredPages; i++)
	{
		g_physical_address page = memoryPhysicalAllocate();
		pagingMapPage(tlsStart + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}

	// Load contents from all loaded objects to memory
	memorySetBytes((uint8_t*) tlsStart, 0, rootObject->tlsMaster.totalSize);

	auto it = hashmapIteratorStart(rootObject->loadedObjects);
	while(hashmapIteratorHasNext(&it))
	{
		g_elf_object* object = hashmapIteratorNext(&it)->value;
		if(object->tlsPart.content)
			memoryCopy((uint8_t*) (tlsStart + object->tlsPart.offset), object->tlsPart.content, object->tlsPart.copysize);
	}
	hashmapIteratorEnd(&it);

	// Update process
	process->tlsMaster.location = tlsStart;
	process->tlsMaster.size = size;
	process->tlsMaster.userThreadOffset = rootObject->tlsMaster.userThreadOffset;
	logDebug("%!   created TLS master: %h, size: %h", "elf", process->tlsMaster.location, process->tlsMaster.size);
}
