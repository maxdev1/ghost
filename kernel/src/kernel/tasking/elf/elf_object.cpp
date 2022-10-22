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
#include "kernel/tasking/elf/elf_tls.hpp"
#include "shared/utils/string.hpp"

g_elf_object_load_result elfObjectLoad(g_task* caller, g_elf_object* parentObject, const char* name,
									   g_fd file, g_virtual_address base, g_address_range_pool* rangeAllocator)
{
	g_elf_object_load_result res;
	res.status = G_SPAWN_STATUS_SUCCESSFUL;

	// Create ELF object
	g_elf_object* object = (g_elf_object*) heapAllocateClear(sizeof(g_elf_object));
	object->name = stringDuplicate(name);
	object->parent = parentObject;
	object->baseAddress = base;
	object->root = (parentObject == 0);
	object->localSymbols = hashmapCreateString<g_elf_symbol_info>(16);
	if(object->root)
	{
		object->loadedObjects = hashmapCreateString<g_elf_object*>(16);
		object->globalSymbols = hashmapCreateString<g_elf_symbol_info>(16);
		object->nextObjectId = 0;
		object->symbolLookupOrderList = 0;
	}
	res.object = object;

	logDebug("%! loading object '%s' (%i) to %h", "elf", name, object->id, base);

	// Check ELF header
	res.validation = elfReadAndValidateHeader(caller, file, &object->header, object->root);
	if(res.validation != G_SPAWN_VALIDATION_SUCCESSFUL)
	{
		logInfo("%! validation failed with status %i when loading object %s", "elf", res.validation, name);
		res.status = object->root ? G_SPAWN_STATUS_FORMAT_ERROR : G_SPAWN_STATUS_DEPENDENCY_ERROR;
		return res;
	}

	// Add to list of already loaded dependencies
	g_elf_object* rootObject = object;
	while(rootObject->parent)
		rootObject = rootObject->parent;

	object->id = rootObject->nextObjectId++;
	hashmapPut(rootObject->loadedObjects, name, object);

	// Add to relocate order list in root object
	object->symbolLookupOrderListNext = rootObject->symbolLookupOrderList;
	rootObject->symbolLookupOrderList = object;

	// Load each program header
	for(uint32_t p = 0; p < object->header.e_phnum; p++)
	{
		Elf32_Phdr phdr;
		uint32_t phdrOffset = object->header.e_phoff + object->header.e_phentsize * p;

		if(!elfReadToMemory(caller, file, phdrOffset, (uint8_t*) &phdr, sizeof(Elf32_Phdr)))
		{
			logInfo("%! failed to read segment header from file %i", "elf", file);
			res.status = G_SPAWN_STATUS_IO_ERROR;
			break;
		}

		if(phdr.p_type == PT_LOAD)
		{
			auto loadResult = elfObjectLoadLoadSegment(caller, file, phdr, base);
			if(loadResult.status == G_SPAWN_STATUS_SUCCESSFUL)
			{
				if(object->startAddress == 0 || loadResult.alignedStart < object->startAddress)
					object->startAddress = loadResult.alignedStart;

				if(loadResult.alignedEnd > object->endAddress)
					object->endAddress = loadResult.alignedEnd;
			}
			else
			{
				res.status = loadResult.status;
				logInfo("%! unable to load PT_LOAD segment from file", "elf");
				break;
			}
		}
		else if(phdr.p_type == PT_TLS)
		{
			res.status = elfTlsLoadData(caller, file, phdr, object, rangeAllocator);
			if(res.status != G_SPAWN_STATUS_SUCCESSFUL)
			{
				logInfo("%! unable to load PT_TLS segment from file", "elf");
				break;
			}
		}
		else if(phdr.p_type == PT_DYNAMIC)
		{
			object->dynamicSection = (Elf32_Dyn*) (base + phdr.p_vaddr);
			logDebug("%!   object has dynamic information %h", "elf", object->dynamicSection);
		}
	}

	// Do analyzation and linking
	if(res.status == G_SPAWN_STATUS_SUCCESSFUL)
	{
		elfObjectInspect(object);

		auto depRes = elfObjectLoadDependencies(caller, object, rangeAllocator);
		if(depRes.status == G_SPAWN_STATUS_SUCCESSFUL)
		{
			res.nextFreeBase = depRes.nextFreeBase;
		}
		else
		{
			res.status = depRes.status;
			return res;
		}

		elfObjectApplyRelocations(caller, file, object);
	}

	return res;
}

g_elf_object_load_segment_result elfObjectLoadLoadSegment(g_task* caller, g_fd file, Elf32_Phdr phdr, g_virtual_address base)
{
	g_address fileStart = base + phdr.p_vaddr;
	g_offset fileSize = phdr.p_filesz;
	g_address fileEnd = fileStart + fileSize;

	g_elf_object_load_segment_result res;
	res.alignedStart = G_PAGE_ALIGN_DOWN(fileStart);
	res.alignedEnd = G_PAGE_ALIGN_UP(fileStart + phdr.p_memsz);

	// Allocate required memory
	uint32_t pages = (res.alignedEnd - res.alignedStart) / G_PAGE_SIZE;
	for(uint32_t i = 0; i < pages; i++)
	{
		g_physical_address page = memoryPhysicalAllocate();
		pagingMapPage(res.alignedStart + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}

	// Zero everything before content
	uint32_t spaceBefore = fileStart - res.alignedStart;
	if(spaceBefore > 0)
		memorySetBytes((void*) res.alignedStart, 0, spaceBefore);

	// Read data to memory
	if(!elfReadToMemory(caller, file, phdr.p_offset, (uint8_t*) fileStart, fileSize))
	{
		res.status = G_SPAWN_STATUS_IO_ERROR;
		return res;
	}

	// Zero everything after content
	uint32_t spaceAfter = res.alignedEnd - fileEnd;
	if(spaceAfter > 0)
		memorySetBytes((void*) fileEnd, 0, spaceAfter);

	res.status = G_SPAWN_STATUS_SUCCESSFUL;
	return res;
}

void elfObjectInspect(g_elf_object* object)
{
	if(!object->dynamicSection)
		return;

	// Find tables that we need
	Elf32_Dyn* it = object->dynamicSection;
	while(it->d_tag)
	{
		switch(it->d_tag)
		{
		case DT_STRTAB:
			object->dynamicStringTable = (char*) (object->baseAddress + it->d_un.d_ptr);
			break;
		case DT_STRSZ:
			object->dynamicStringTableSize = it->d_un.d_val;
			break;
		case DT_HASH:
			object->dynamicSymbolHashTable = (Elf32_Word*) (object->baseAddress + it->d_un.d_ptr);
			// The number of symbol table entries should equal nchain; so symbol table indexes also select chain table entries.
			object->dynamicSymbolTableSize = object->dynamicSymbolHashTable[1];
			break;
		case DT_SYMTAB:
			object->dynamicSymbolTable = (Elf32_Sym*) (object->baseAddress + it->d_un.d_ptr);
			break;
		case DT_INIT:
			object->init = (void (*)())(object->baseAddress + it->d_un.d_ptr);
			break;
		case DT_FINI:
			object->fini = (void (*)())(object->baseAddress + it->d_un.d_ptr);
			break;
		case DT_PREINIT_ARRAY:
			object->preinitArray = (void (**)())(object->baseAddress + it->d_un.d_ptr);
			break;
		case DT_PREINIT_ARRAYSZ:
			object->preinitArraySize = it->d_un.d_val / sizeof(uintptr_t);
			break;
		case DT_INIT_ARRAY:
			object->initArray = (void (**)())(object->baseAddress + it->d_un.d_ptr);
			break;
		case DT_INIT_ARRAYSZ:
			object->initArraySize = it->d_un.d_val / sizeof(uintptr_t);
			break;
		case DT_FINI_ARRAY:
			object->finiArray = (void (**)())(object->baseAddress + it->d_un.d_ptr);
			break;
		case DT_FINI_ARRAYSZ:
			object->finiArraySize = it->d_un.d_val / sizeof(uintptr_t);
			break;
		}
		it++;
	}

	// Read dependencies
	object->dependencies = 0;
	it = object->dynamicSection;
	while(it->d_tag)
	{
		if(it->d_tag == DT_NEEDED)
		{
			g_elf_dependency* dep = (g_elf_dependency*) heapAllocate(sizeof(g_elf_dependency));
			dep->name = stringDuplicate(object->dynamicStringTable + it->d_un.d_val);
			dep->next = object->dependencies;
			object->dependencies = dep;
		}
		it++;
	}

	// Put symbols into root objects dynamic symbol table. This happens in load order.
	if(object->dynamicSymbolTable)
	{
		g_elf_object* rootObject = object;
		while(rootObject->parent)
			rootObject = rootObject->parent;

		uint32_t pos = 0;
		Elf32_Sym* it = object->dynamicSymbolTable;
		while(pos < object->dynamicSymbolTableSize)
		{
			const char* symbol = (const char*) (object->dynamicStringTable + it->st_name);
			if(it->st_shndx)
			{
				g_elf_symbol_info symbolInfo;
				symbolInfo.object = object;
				symbolInfo.absolute = object->baseAddress + it->st_value;
				symbolInfo.value = it->st_value;
				hashmapPut<const char*, g_elf_symbol_info>(object->localSymbols, symbol, symbolInfo);

				if(hashmapGetEntry<const char*, g_elf_symbol_info>(rootObject->globalSymbols, symbol) == 0)
					hashmapPut<const char*, g_elf_symbol_info>(rootObject->globalSymbols, symbol, symbolInfo);
			}

			it++;
			pos++;
		}
	}
}

g_elf_object_load_result elfObjectLoadDependencies(g_task* caller, g_elf_object* object, g_address_range_pool* rangeAllocator)
{
	g_elf_object_load_result res;
	res.status = G_SPAWN_STATUS_SUCCESSFUL;
	res.nextFreeBase = object->endAddress;

	for(g_elf_dependency* dependency = object->dependencies;
		dependency;
		dependency = dependency->next)
	{
		if(elfObjectIsDependencyLoaded(object, dependency->name))
			continue;

		auto depRes = elfObjectLoadDependency(caller, object, dependency->name, res.nextFreeBase, rangeAllocator);
		res.nextFreeBase = depRes.nextFreeBase;

		if(depRes.status != G_SPAWN_STATUS_SUCCESSFUL)
		{
			res.status = depRes.status;
			logInfo("%!   -> failed to load dependency %s with status %i", "elf", dependency->name, depRes.status);
			break;
		}
	}

	return res;
}

void elfObjectApplyRelocations(g_task* caller, g_fd file, g_elf_object* object)
{
	g_elf_object* rootObject = object;
	while(rootObject->parent)
		rootObject = rootObject->parent;

	for(uint32_t p = 0; p < object->header.e_shnum * object->header.e_shentsize; p += object->header.e_shentsize)
	{
		Elf32_Shdr shdr;
		if(!elfReadToMemory(caller, file, object->header.e_shoff + p, (uint8_t*) &shdr, object->header.e_shentsize))
		{
			logInfo("%! failed to read section header from file", "elf");
			break;
		}

		if(shdr.sh_type != SHT_REL)
		{
			continue;
		}

		Elf32_Rel* entry = (Elf32_Rel*) (object->baseAddress + shdr.sh_addr);
		while(entry < (Elf32_Rel*) (object->baseAddress + shdr.sh_addr + shdr.sh_size))
		{
			uint32_t symbolIndex = ELF32_R_SYM(entry->r_info);
			uint8_t type = ELF32_R_TYPE(entry->r_info);

			g_address cS;
			g_address cP = object->baseAddress + entry->r_offset;

			Elf32_Word symbolSize;
			const char* symbolName = 0;
			g_elf_symbol_info symbolInfo;

			// Symbol lookup
			if(type == R_386_32 || type == R_386_PC32 ||
			   type == R_386_GLOB_DAT || type == R_386_JMP_SLOT ||
			   type == R_386_GOTOFF || type == R_386_TLS_TPOFF ||
			   type == R_386_TLS_DTPMOD32 || type == R_386_TLS_DTPOFF32 ||
			   type == R_386_COPY)
			{
				Elf32_Sym* symbol = &object->dynamicSymbolTable[symbolIndex];
				symbolName = &object->dynamicStringTable[symbol->st_name];
				symbolSize = symbol->st_size;

				bool symbolFound = false;
				if(type == R_386_COPY)
				{
					auto symbolLookupEntry = rootObject->symbolLookupOrderList;
					while(symbolLookupEntry)
					{
						auto entry = hashmapGetEntry(symbolLookupEntry->localSymbols, symbolName);
						if(entry)
						{
							symbolInfo = entry->value;
							symbolFound = true;
							break;
						}
						symbolLookupEntry = symbolLookupEntry->symbolLookupOrderListNext;
					}
				}
				else
				{
					auto entry = hashmapGetEntry(rootObject->globalSymbols, symbolName);
					if(entry)
					{
						symbolFound = true;
						symbolInfo = entry->value;
					}
				}

				if(symbolFound)
				{
					cS = symbolInfo.absolute;
				}
				else
				{
					if(ELF32_ST_BIND(symbol->st_info) != STB_WEAK)
						logDebug("%!     missing symbol '%s' (%h, bind: %i)", "elf", symbolName, cP, ELF32_ST_BIND(symbol->st_info));

					cS = 0;
				}
			}

			if(type == R_386_32)
			{
				int32_t cA = *((int32_t*) cP);
				*((uint32_t*) cP) = cS + cA;
			}
			else if(type == R_386_PC32)
			{
				int32_t cA = *((int32_t*) cP);
				*((uint32_t*) cP) = cS + cA - cP;
			}
			else if(type == R_386_COPY)
			{
				if(cS)
					memoryCopy((void*) cP, (void*) cS, symbolSize);
			}
			else if(type == R_386_GLOB_DAT)
			{
				*((uint32_t*) cP) = cS;
			}
			else if(type == R_386_JMP_SLOT)
			{
				*((uint32_t*) cP) = cS;
			}
			else if(type == R_386_RELATIVE)
			{
				uint32_t cB = object->baseAddress;
				int32_t cA = *((int32_t*) cP);
				*((uint32_t*) cP) = cB + cA;
			}
			else if(type == R_386_TLS_TPOFF)
			{
				/**
				 * For TLS_TPOFF we insert the offset relative to the g_user_threadlocal which is put
				 * into the segment referenced in GS.
				 */
				if(cS)
					*((uint32_t*) cP) = symbolInfo.object->tlsPart.offset - rootObject->tlsMaster.userThreadOffset + symbolInfo.value;
			}
			else if(type == R_386_TLS_DTPMOD32)
			{
				/**
				 * DTPMOD32 expects the module ID to be written which will be passed to ___tls_get_addr.
				 */
				if(cS)
					*((uint32_t*) cP) = symbolInfo.object->id;
			}
			else if(type == R_386_TLS_DTPOFF32)
			{
				/**
				 * DTPOFF32 expects the symbol offset to be written which will be passed to ___tls_get_addr.
				 */
				if(cS)
					*((uint32_t*) cP) = symbolInfo.object->tlsPart.offset - rootObject->tlsMaster.userThreadOffset + symbolInfo.value;
			}

			entry++;
		}
	}
}

void elfObjectDestroy(g_elf_object* elfObject)
{
	if(elfObject->loadedObjects)
	{
		auto loadedIter = hashmapIteratorStart(elfObject->loadedObjects);
		while(hashmapIteratorHasNext(&loadedIter))
		{
			auto loadedObject = hashmapIteratorNext(&loadedIter);

			if(loadedObject->value != elfObject)
				elfObjectDestroy(loadedObject->value);
		}
		hashmapIteratorEnd(&loadedIter);

		hashmapDestroy(elfObject->loadedObjects);
	}

	hashmapDestroy(elfObject->localSymbols);

	if(elfObject->globalSymbols)
		hashmapDestroy(elfObject->globalSymbols);

	heapFree(elfObject);
}

bool elfObjectIsDependencyLoaded(g_elf_object* object, const char* name)
{
	g_elf_object* rootObject = object;
	while(rootObject->parent)
		rootObject = rootObject->parent;

	return hashmapGet(rootObject->loadedObjects, name, (g_elf_object*) nullptr) != nullptr;
}

g_elf_object_load_result elfObjectLoadDependency(g_task* caller, g_elf_object* parentObject, const char* name,
												 g_virtual_address baseAddress, g_address_range_pool* rangeAllocator)
{
	g_fd fd = elfObjectOpenDependency(caller, name);
	if(fd == G_FD_NONE)
		return {status : G_SPAWN_STATUS_IO_ERROR};

	auto dependencyData = elfObjectLoad(caller, parentObject, name, fd, baseAddress, rangeAllocator);
	filesystemClose(caller->process->id, fd, true);
	return dependencyData;
}

g_fd elfObjectOpenDependency(g_task* caller, const char* name)
{
	const char* prefix = "/system/lib/";
	char* absolutePath = (char*) heapAllocate(stringLength(prefix) + stringLength(name) + 1);
	stringConcat(prefix, name, absolutePath);

	auto findRes = filesystemFind(0, absolutePath);
	if(findRes.status != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! unable to resolve dependency %s (status %i)", "elf", absolutePath, findRes.status);
		heapFree(absolutePath);
		return G_FD_NONE;
	}

	g_fd fd;
	g_fs_open_status openStatus = filesystemOpenNodeFd(findRes.file, G_FILE_FLAG_MODE_BINARY | G_FILE_FLAG_MODE_READ, caller->process->id, &fd);
	if(openStatus != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! unable to open dependency %s", "elf", absolutePath);
		heapFree(absolutePath);
		return G_FD_NONE;
	}

	heapFree(absolutePath);
	return fd;
}
