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


g_spawn_status elfTlsLoadData(g_task* caller, g_fd file, elf32_phdr* phdr, g_elf_object* object, g_address_range_pool* rangeAllocator)
{
	uint32_t bytesToCopy = phdr->p_filesz;

	/* Read TLS content to a buffer */
	uint8_t* tlsContentBuffer = (uint8_t*) heapAllocate(bytesToCopy);
	if(!elfReadToMemory(caller, file, phdr->p_offset, (uint8_t*) tlsContentBuffer, bytesToCopy))
	{
		logInfo("%! unable to read TLS segment from file", "elf");
		heapFree(tlsContentBuffer);
		return G_SPAWN_STATUS_IO_ERROR;
	}

	/* Write object information */
	object->tlsMaster.content = tlsContentBuffer;
	object->tlsMaster.alignment = phdr->p_align;
	object->tlsMaster.copysize = phdr->p_filesz;
	object->tlsMaster.totalsize = phdr->p_memsz;

	/* Calculate offset in TLS master image */
	if(object->executable) {
		/* Executable gets offset 0 and is followed with the g_user_thread */
		object->tlsMaster.offset = 0;
		uint32_t size = G_ALIGN_UP(object->tlsMaster.totalsize, object->tlsMaster.alignment);
		object->tlsMasterTotalSize = size + sizeof(g_user_thread);
		object->tlsMasterUserThreadOffset = size;
	}
	else {
		/* Shared libraries just get the next free position */
		g_elf_object* executableObject = object;
		while(executableObject->parent) executableObject = executableObject->parent;
		object->tlsMaster.offset = executableObject->tlsMasterTotalSize;
		executableObject->tlsMasterTotalSize += G_ALIGN_UP(object->tlsMaster.totalsize, object->tlsMaster.alignment);
	}

	return G_SPAWN_STATUS_SUCCESSFUL;
}

void elf32TlsCreateMasterImage(g_task* caller, g_fd file, g_process* process, g_elf_object* executableObject)
{
	/* Allocate memory */
	uint32_t sizeInPages = G_PAGE_ALIGN_UP(executableObject->tlsMasterTotalSize);
	uint32_t requiredPages = sizeInPages / G_PAGE_SIZE;
	g_virtual_address tlsStart = addressRangePoolAllocate(process->virtualRangePool, requiredPages);
	for(uint32_t i = 0; i < requiredPages; i++)
	{
		g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pagingMapPage(tlsStart + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		pageReferenceTrackerIncrement(page);
	}


	/* Copy all contents */
	memorySetBytes((uint8_t*) tlsStart, 0, executableObject->tlsMasterTotalSize);

	logDebug("%!   write TLS master image", "elf");
	auto it = hashmapIteratorStart(executableObject->loadedObjects);
	while(hashmapIteratorHasNext(&it))
	{
		g_elf_object* object = hashmapIteratorNext(&it)->value;
		if(object->tlsMaster.content)
		{
			memoryCopy((uint8_t*) (tlsStart + object->tlsMaster.offset), object->tlsMaster.content, object->tlsMaster.copysize);
			logDebug("%!    [%h, size %h, binary %s]", "elf", tlsStart + object->tlsMaster.offset, object->tlsMaster.totalsize, object->name);
		}
	}
	hashmapIteratorEnd(&it);

	/* Update process */
	process->tlsMaster.location = tlsStart;
	process->tlsMaster.size = sizeInPages;
	process->tlsMaster.userThreadOffset = executableObject->tlsMasterUserThreadOffset;
	logDebug("%!   created TLS master: %h, size: %h", "elf", process->tlsMaster.location, process->tlsMaster.size);
}
