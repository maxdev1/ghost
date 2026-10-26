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
#include "kernel/memory/constants.hpp"
#include "kernel/tasking/elf/elf_loader.hpp"
#include "kernel/logger/logger.hpp"

g_spawn_status elfTlsLoadData(g_fd file, Elf64_Phdr phdr, g_elf_object* object)
{
    const uint64_t limit = G_USER_VIRTUAL_RANGES_END - G_USER_VIRTUAL_RANGES_START;
    const uint64_t alignment = phdr.p_align ? phdr.p_align : 1;
    if(object->tlsPart.content || phdr.p_filesz > phdr.p_memsz || phdr.p_memsz > limit ||
       alignment > limit || (alignment & (alignment - 1)) ||
       phdr.p_offset > G_ADDRESS_MAX - phdr.p_filesz)
        return G_SPAWN_STATUS_FORMAT_ERROR;
    auto root = object;
    while(root->parent)
        root = root->parent;
    // Executables without PT_TLS still need their thread-control slot before
    // any library TLS is appended; otherwise the two objects overlap at offset 0.
    if(!object->root && root->tlsMaster.totalSize == 0)
        root->tlsMaster.totalSize = sizeof(g_user_threadlocal);
    const auto offset = object->root ? 0 : G_ALIGN_UP((uint64_t)root->tlsMaster.totalSize, alignment);
    const auto threadAlignment = alignment > alignof(g_user_threadlocal) ? alignment : alignof(g_user_threadlocal);
    const auto end = object->root ? G_ALIGN_UP(phdr.p_memsz, threadAlignment) + sizeof(g_user_threadlocal)
                                  : offset + G_ALIGN_UP(phdr.p_memsz, alignment);
    if(end > limit)
        return G_SPAWN_STATUS_MEMORY_ERROR;
    auto content = (uint8_t*) heapTryAllocate(phdr.p_filesz);
    if(!content)
        return G_SPAWN_STATUS_MEMORY_ERROR;
    if(!filesystemReadToMemory(file, phdr.p_offset, content, phdr.p_filesz))
    {
        heapFree(content);
        return G_SPAWN_STATUS_IO_ERROR;
    }
    object->tlsPart.content = content;
    object->tlsPart.alignment = alignment;
    object->tlsPart.copysize = phdr.p_filesz;
    object->tlsPart.totalsize = phdr.p_memsz;
    object->tlsPart.offset = offset;
    root->tlsMaster.totalSize = end;
    if(object->root)
        root->tlsMaster.userThreadOffset = end - sizeof(g_user_threadlocal);
    return G_SPAWN_STATUS_SUCCESSFUL;
}

bool elfTlsCreateMasterImage(g_fd file, g_process* process, g_elf_object* rootObject)
{
	// Allocate memory
    if(rootObject->tlsMaster.totalSize == 0)
        rootObject->tlsMaster.totalSize = sizeof(g_user_threadlocal);
    if(rootObject->tlsMaster.userThreadOffset > rootObject->tlsMaster.totalSize ||
       sizeof(g_user_threadlocal) > rootObject->tlsMaster.totalSize - rootObject->tlsMaster.userThreadOffset)
        return false;
    uint32_t size = G_PAGE_ALIGN_UP(rootObject->tlsMaster.totalSize);
	uint32_t requiredPages = size / G_PAGE_SIZE;
	g_virtual_address tlsStart = addressRangePoolAllocate(process->virtualRangePool, requiredPages,
	                                                     G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL);
    if(!tlsStart)
        return false;
    for(uint32_t i = 0; i < requiredPages; i++)
	{
		g_physical_address page = memoryPhysicalTryAllocate();
        if(!page)
            return false;
		memorySetBytes((void*) G_MEM_PHYS_TO_VIRT(page), 0, G_PAGE_SIZE);
        if(!pagingMapPage(tlsStart + i * G_PAGE_SIZE, page, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT))
        {
            memoryPhysicalFree(page);
            return false;
        }
	}

	// Load contents from all loaded objects to memory
	memorySetBytes((uint8_t*) tlsStart, 0, rootObject->tlsMaster.totalSize);

	auto it = hashmapIteratorStart(rootObject->loadedObjects);
	while(hashmapIteratorHasNext(&it))
	{
		g_elf_object* object = hashmapIteratorNext(&it)->value;
		if(object->tlsPart.content)
			memoryCopy((uint8_t*) (tlsStart + object->tlsPart.offset), object->tlsPart.content,
			           object->tlsPart.copysize);
	}
	hashmapIteratorEnd(&it);

	// Update process
	process->tlsMaster.location = tlsStart;
	process->tlsMaster.size = size;
	process->tlsMaster.userThreadOffset = rootObject->tlsMaster.userThreadOffset;
	logDebug("%!   created TLS master: %h, size: %h, uTO: %x", "elf", process->tlsMaster.location, process->tlsMaster.size, process->tlsMaster.userThreadOffset);
    return true;
}
