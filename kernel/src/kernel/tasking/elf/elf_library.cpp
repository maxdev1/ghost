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


g_spawn_status elfLibraryLoad(g_task* caller, g_elf_object* parentObject, const char* name, g_virtual_address baseAddress,
	g_address_range_pool* rangeAllocator, g_virtual_address* outNextBase, g_elf_object** outObject)
{
	/* Check if dependency is already loaded */
	g_elf_object* executableObject = parentObject;
	while(executableObject->parent)
	{
		executableObject = executableObject->parent;
	}
	if(hashmapGet(executableObject->loadedObjects, name, (g_elf_object*) 0))
	{
		return G_SPAWN_STATUS_DEPENDENCY_DUPLICATE;
	}

	/* Open and load library */
	g_fd fd = elfLibraryOpen(caller, name);
	if(fd == G_FD_NONE)
	{
		return G_SPAWN_STATUS_DEPENDENCY_ERROR;
	}
	g_spawn_status status = elfObjectLoad(caller, parentObject, name, fd, baseAddress, rangeAllocator, outNextBase, outObject);
	filesystemClose(caller->process->id, fd, true);
	return status;
}

g_virtual_address elfLibraryLoadDependencies(g_task* caller, g_elf_object* object, g_address_range_pool* rangeAllocator)
{
	g_virtual_address nextBase = object->endAddress;

	g_elf_dependency* dependency = object->dependencies;
	while(dependency)
	{
		g_elf_object* dependencyObject;
		g_spawn_status status = elfLibraryLoad(caller, object, dependency->name, nextBase, rangeAllocator, &nextBase, &dependencyObject);

		if(status == G_SPAWN_STATUS_SUCCESSFUL)
		{
			logDebug("%!   successfully loaded dependency", "elf");

		} else if(status == G_SPAWN_STATUS_DEPENDENCY_DUPLICATE)
		{
			logDebug("%!   duplicate dependency ignored: %s -> %s", "elf", object->name, dependency->name);

		} else
		{
			status = G_SPAWN_STATUS_DEPENDENCY_ERROR;
			logInfo("%!   -> failed to load dependency %s", "elf", dependency->name);
			break;
		}
		dependency = dependency->next;
	}

	return nextBase;
}

g_fd elfLibraryOpen(g_task* caller, const char* name)
{
	const char* prefix = "/system/lib/";
	char* absolutePath = (char*) heapAllocate(stringLength(prefix) + stringLength(name) + 1);
	stringConcat(prefix, name, absolutePath);

	g_fs_node* file;
	bool foundAllButLast;
	g_fs_node* lastFoundParent;
	const char* filenameStart;

	g_fs_open_status findStatus = filesystemFind(0, absolutePath, &file, &foundAllButLast, &lastFoundParent, &filenameStart);
	if(findStatus != G_FS_OPEN_SUCCESSFUL) {
		logInfo("%! unable to resolve dependency %s (status %i)", "elf", absolutePath, findStatus);
		heapFree(absolutePath);
		return G_FD_NONE;
	}

	g_fd fd;
	g_fs_open_status openStatus = filesystemOpen(file, G_FILE_FLAG_MODE_BINARY | G_FILE_FLAG_MODE_READ, caller, &fd);
	if(openStatus != G_FS_OPEN_SUCCESSFUL) {
		logInfo("%! unable to open dependency %s", "elf", absolutePath);
		heapFree(absolutePath);
		return G_FD_NONE;
	}

	heapFree(absolutePath);
	return fd;
}

void elfObjectApplyRelocations(g_task* caller, g_fd file, g_elf_object* object)
{
	logDebug("%!   applying relocations for '%s'", "elf", object->name);
	g_elf_object* executableObject = object;
	while(executableObject->parent) {
		executableObject = executableObject->parent;
	}
	
	for(uint32_t p = 0; p < object->header.e_shnum * object->header.e_shentsize; p += object->header.e_shentsize) {
		
		elf32_shdr sectionHeader;
		if(!elfReadToMemory(caller, file, object->header.e_shoff + p, (uint8_t*) &sectionHeader, object->header.e_shentsize)) {
			logInfo("%! failed to read section header from file", "elf");
			break;
		}

		if(sectionHeader.sh_type != SHT_REL) {
			continue;
		}

		elf32_rel* entry = (elf32_rel*) (object->baseAddress + sectionHeader.sh_addr);
		while(entry < (elf32_rel*) (object->baseAddress + sectionHeader.sh_addr + sectionHeader.sh_size)) {
			uint32_t symbolIndex = ELF32_R_SYM(entry->r_info);
			uint8_t type = ELF32_R_TYPE(entry->r_info);

			uint32_t cS;
			g_virtual_address cP = object->baseAddress + entry->r_offset;

			elf32_word symbolSize;
			const char* symbolName = 0;
			g_elf_symbol_info symbolInfo;

			if (type == R_386_32 || type == R_386_PC32 || type == R_386_GLOB_DAT || type == R_386_JMP_SLOT || type == R_386_GOTOFF ||
				type == R_386_TLS_TPOFF || type == R_386_TLS_DTPMOD32 || type == R_386_TLS_DTPOFF32 || type == R_386_COPY)
			{
				elf32_sym* symbol = &object->dynamicSymbolTable[symbolIndex];
				symbolName = &object->dynamicStringTable[symbol->st_name];
				symbolSize = symbol->st_size;

				/* Symbol lookup */
				bool symbolFound = false;
				if(type == R_386_COPY)
				{
					g_elf_object* relocateOrderNext = executableObject->relocateOrderFirst;
					while(relocateOrderNext) {
						auto entry = hashmapGetEntry<const char*, g_elf_symbol_info>(relocateOrderNext->localSymbols, symbolName);
						if(entry)
						{
							symbolInfo = entry->value;
							symbolFound = true;
							break;
						}
						relocateOrderNext = relocateOrderNext->relocateOrderNext;
					}
				} else
				{
					auto entry = hashmapGetEntry<const char*, g_elf_symbol_info>(executableObject->globalSymbols, symbolName);
					if(entry) {
						symbolFound = true;
						symbolInfo = entry->value;
					}
				}

				if(symbolFound) {
					cS = symbolInfo.absolute;
				}
				else {
					if(ELF32_ST_BIND(symbol->st_info) != STB_WEAK) {
						logInfo("%!     missing symbol '%s' (%h, bind: %i)", "elf", symbolName, cP, ELF32_ST_BIND(symbol->st_info));
					}
					cS = 0;
				}
			}

			if(type == R_386_32)
			{
				int32_t cA = *((int32_t*) cP);
				*((uint32_t*) cP) = cS + cA;

			} else if(type == R_386_PC32)
			{
				int32_t cA = *((int32_t*) cP);
				*((uint32_t*) cP) = cS + cA - cP;

			} else if(type == R_386_COPY)
			{
				if(cS)
				{
					memoryCopy((void*) cP, (void*) cS, symbolSize);
					// logInfo("Copy %i from %x to %x (%s)", symbolSize, cS, cP, symbolName);
				}

			} else if(type == R_386_GLOB_DAT)
			{
				*((uint32_t*) cP) = cS;
				// logInfo("Set glob dat %x to %x (%s)", cP, cS, symbolName);
				
			} else if(type == R_386_JMP_SLOT)
			{
				*((uint32_t*) cP) = cS;

			} else if(type == R_386_RELATIVE)
			{
				uint32_t cB = object->baseAddress;
				int32_t cA = *((int32_t*) cP);
				*((uint32_t*) cP) = cB + cA;

			} else if(type == R_386_TLS_TPOFF)
			{
				if(cS)
				{
					/**
					 * For TLS_TPOFF we insert the offset relative to the g_user_thread which is put
					 * into the segment referenced in GS.
					 */
					*((uint32_t*) cP) = symbolInfo.object->tlsMaster.offset - executableObject->tlsMasterUserThreadOffset + symbolInfo.value;
					logDebug("%!      R_386_TLS_TPOFF: %s, %h = %h", "elf", symbolName, cP, *((uint32_t*) cP));
				}

			} else if(type == R_386_TLS_DTPMOD32)
			{
				if(cS)
				{
					/**
					 * DTPMOD32 expects the module ID to be written which will be passed to ___tls_get_addr.
					 */
					*((uint32_t*) cP) = symbolInfo.object->id;
					logDebug("%!      R_386_TLS_DTPMOD32: %s, %h = %h", "elf", symbolName, cP, *((uint32_t*) cP));
				}
				
			} else if(type == R_386_TLS_DTPOFF32)
			{
				if(cS)
				{
					/**
					 * DTPOFF32 expects the symbol offset to be written which will be passed to ___tls_get_addr.
					 */
					*((uint32_t*) cP) = symbolInfo.object->tlsMaster.offset - executableObject->tlsMasterUserThreadOffset + symbolInfo.value;
					logDebug("%!      R_386_TLS_DTPOFF32: %s, %h = %h", "elf", symbolName, cP, *((uint32_t*) cP));
				}

			} else
			{
				logDebug("%!     binary contains unhandled relocation: %i", "elf", type);
			}

			entry++;
		}
	}

}
