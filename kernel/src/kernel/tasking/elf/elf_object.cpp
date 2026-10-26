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
#include "kernel/tasking/elf/elf_tls.hpp"
#include "kernel/utils/string.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/utils/debug.hpp"

static bool _elfObjectValidateLoadRange(Elf64_Phdr phdr, g_virtual_address base)
{
	// The loader's copy/zero helpers use signed 32-bit lengths. Images also
	// must stay below the separate pool used for stacks, TLS and shared ranges.
	if(phdr.p_filesz > phdr.p_memsz || phdr.p_memsz > INT32_MAX ||
	   phdr.p_offset > G_ADDRESS_MAX - phdr.p_filesz ||
	   base > G_USER_MAXIMUM_HEAP_BREAK || phdr.p_vaddr > G_USER_MAXIMUM_HEAP_BREAK - base)
		return false;
	const g_virtual_address start = base + phdr.p_vaddr;
	return phdr.p_memsz == 0 || (start >= G_PAGE_SIZE && phdr.p_memsz <= G_USER_MAXIMUM_HEAP_BREAK - start);
}

g_elf_object_load_result elfObjectLoad(g_elf_object* parentObject, const char* name, g_fd file, g_virtual_address base)
{
	g_elf_object_load_result result{};
	result.status = G_SPAWN_STATUS_SUCCESSFUL;

	// Create ELF object
	auto object = (g_elf_object*) heapAllocateClear(sizeof(g_elf_object));
	object->name = stringDuplicate(name);
	object->parent = parentObject;
	object->baseAddress = base;
	object->root = (parentObject == 0);
	if(object->root)
	{
		object->loadedObjects = hashmapCreateString<g_elf_object*>(16);
		object->nextObjectId = 0;
		object->symbolLookupOrderList = 0;
	}
	result.object = object;

	logDebug("%! loading object '%s' (%i) to %h (fd %i)", "elf", name, object->id, base, file);

	// Add to list of already loaded dependencies
	g_elf_object* rootObject = object;
	while(rootObject->parent)
		rootObject = rootObject->parent;

	object->id = rootObject->nextObjectId++;
	hashmapPut(rootObject->loadedObjects, name, object);

	// Add to relocate order list in root object
	// Keep both load orders: normal relocations prefer the first definition,
	// while COPY relocations retain the reverse-load-order lookup.
	if(rootObject->symbolLookupOrderList)
		rootObject->symbolLookupOrderList->symbolLoadOrderNext = object;
	object->symbolLookupOrderListNext = rootObject->symbolLookupOrderList;
	rootObject->symbolLookupOrderList = object;

	// Check ELF header
	result.validation = elfReadAndValidateHeader(file, &object->header, object->root);
	if(result.validation != G_SPAWN_VALIDATION_SUCCESSFUL)
	{
		logInfo("%! validation failed with status %i when loading object %s", "elf", result.validation, name);
		result.status = object->root ? G_SPAWN_STATUS_FORMAT_ERROR : G_SPAWN_STATUS_DEPENDENCY_ERROR;
		return result;
	}

	// Load each program header
	for(Elf64_Half p = 0; p < object->header.e_phnum; p++)
	{
		Elf64_Phdr phdr;
		g_address phdrOffset = object->header.e_phoff + object->header.e_phentsize * p;

		if(!filesystemReadToMemory(file, phdrOffset, (uint8_t*) &phdr, sizeof(Elf64_Phdr)))
		{
			logInfo("%! failed to read segment header from file %i", "elf", file);
			result.status = G_SPAWN_STATUS_IO_ERROR;
			break;
		}

		if(phdr.p_type == PT_LOAD)
		{
			if(!_elfObjectValidateLoadRange(phdr, base))
			{
				result.status = G_SPAWN_STATUS_FORMAT_ERROR;
				break;
			}
			auto fileStart = base + phdr.p_vaddr;
			auto alignedStart = G_PAGE_ALIGN_DOWN(fileStart);
			auto alignedEnd = G_PAGE_ALIGN_UP(fileStart + phdr.p_memsz);

			if(object->root)
			{
				auto loadResult = elfObjectLoadLoadSegment(file, phdr, base);
				if(loadResult != G_SPAWN_STATUS_SUCCESSFUL)
				{
					result.status = loadResult;
					logInfo("%! unable to load PT_LOAD segment from file", "elf");
					break;
				}
			}
			else
			{
				memoryOnDemandMapFile(taskingGetCurrentTask()->process, file, phdr.p_offset, fileStart, phdr.p_filesz,
				                      phdr.p_memsz);
			}

			if(object->startAddress == 0 || alignedStart < object->startAddress)
				object->startAddress = alignedStart;

			if(alignedEnd > object->endAddress)
				object->endAddress = alignedEnd;
		}
		else if(phdr.p_type == PT_TLS)
		{
			result.status = elfTlsLoadData(file, phdr, object);
			if(result.status != G_SPAWN_STATUS_SUCCESSFUL)
			{
				logInfo("%! unable to load PT_TLS segment from file", "elf");
				break;
			}
		}
		else if(phdr.p_type == PT_DYNAMIC)
		{
			object->dynamicSection = (Elf64_Dyn*) (base + phdr.p_vaddr);
			logDebug("%!   object has dynamic information %h", "elf", object->dynamicSection);
		}
	}

	// Do analyzation and linking
	if(result.status == G_SPAWN_STATUS_SUCCESSFUL)
	{
		elfObjectInspect(object);

		auto depRes = elfObjectLoadDependencies(object);
		if(depRes.status == G_SPAWN_STATUS_SUCCESSFUL)
		{
			result.nextFreeBase = depRes.nextFreeBase;
		}
		else
		{
			result.status = depRes.status;
			return result;
		}

		elfObjectApplyRelocations(file, object);
	}

	return result;
}

g_spawn_status elfObjectLoadLoadSegment(g_fd file, Elf64_Phdr phdr, g_virtual_address base)
{
	if(!_elfObjectValidateLoadRange(phdr, base))
		return G_SPAWN_STATUS_FORMAT_ERROR;
	g_address fileStart = base + phdr.p_vaddr;
	g_offset fileSize = phdr.p_filesz;
	g_address fileEnd = fileStart + fileSize;

	g_address alignedStart = G_PAGE_ALIGN_DOWN(fileStart);
	g_address alignedEnd = G_PAGE_ALIGN_UP(fileStart + phdr.p_memsz);

	// Allocate required memory
	uint32_t pages = (alignedEnd - alignedStart) / G_PAGE_SIZE;
	for(uint32_t i = 0; i < pages; i++)
	{
		auto address = alignedStart + i * G_PAGE_SIZE;
		if(pagingVirtualToPageEntry(address) & G_PAGE_PRESENT)
			continue;
		g_physical_address page = memoryPhysicalTryAllocate();
		if(!page)
			return G_SPAWN_STATUS_MEMORY_ERROR;
		if(pagingMapPage(address, page, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT))
			memorySetBytes((void*) address, 0, G_PAGE_SIZE);
		else
		{
			memoryPhysicalFree(page);
			if(!(pagingVirtualToPageEntry(address) & G_PAGE_PRESENT))
				return G_SPAWN_STATUS_MEMORY_ERROR;
		}
	}

	// Read data to memory
	if(!filesystemReadToMemory(file, phdr.p_offset, (uint8_t*) fileStart, fileSize))
	{
		return G_SPAWN_STATUS_IO_ERROR;
	}

	// Zero BSS only; adjacent segments may share the rest of a boundary page.
	if(phdr.p_memsz > fileSize)
		memorySetBytes((void*) fileEnd, 0, phdr.p_memsz - fileSize);

	return G_SPAWN_STATUS_SUCCESSFUL;
}

void elfObjectInspect(g_elf_object* object)
{
	if(!object->dynamicSection)
		return;

	// Find tables that we need
	Elf64_Dyn* it = object->dynamicSection;
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
				object->dynamicSymbolHashTable = (Elf64_Word*) (object->baseAddress + it->d_un.d_ptr);
			// The number of symbol table entries should equal nchain; so symbol table indexes also select chain table entries.
				object->dynamicSymbolTableSize = object->dynamicSymbolHashTable[1];
				break;
			case DT_SYMTAB:
				object->dynamicSymbolTable = (Elf64_Sym*) (object->baseAddress + it->d_un.d_ptr);
				break;
			case DT_INIT:
				object->init = (void (*)()) (object->baseAddress + it->d_un.d_ptr);
				break;
			case DT_FINI:
				object->fini = (void (*)()) (object->baseAddress + it->d_un.d_ptr);
				break;
			case DT_PREINIT_ARRAY:
				object->preinitArray = (void (**)()) (object->baseAddress + it->d_un.d_ptr);
				break;
			case DT_PREINIT_ARRAYSZ:
				object->preinitArraySize = it->d_un.d_val / sizeof(uintptr_t);
				break;
			case DT_INIT_ARRAY:
				object->initArray = (void (**)()) (object->baseAddress + it->d_un.d_ptr);
				break;
			case DT_INIT_ARRAYSZ:
				object->initArraySize = it->d_un.d_val / sizeof(uintptr_t);
				break;
			case DT_FINI_ARRAY:
				object->finiArray = (void (**)()) (object->baseAddress + it->d_un.d_ptr);
				break;
			case DT_FINI_ARRAYSZ:
				object->finiArraySize = it->d_un.d_val / sizeof(uintptr_t);
				break;
		}
		it++;
	}

	// Read dependencies
	object->dependencies = nullptr;
	it = object->dynamicSection;
	while(it->d_tag)
	{
		if(it->d_tag == DT_NEEDED)
		{
			auto dependency = (g_elf_dependency*) heapAllocate(sizeof(g_elf_dependency));
			dependency->name = stringDuplicate(object->dynamicStringTable + it->d_un.d_val);
			dependency->next = object->dependencies;
			object->dependencies = dependency;
		}
		it++;
	}
}

g_elf_object_load_result elfObjectLoadDependencies(g_elf_object* object)
{
	g_elf_object_load_result result{};
	result.status = G_SPAWN_STATUS_SUCCESSFUL;
	result.nextFreeBase = object->endAddress;

	for(g_elf_dependency* dependency = object->dependencies;
	    dependency;
	    dependency = dependency->next)
	{
		if(elfObjectIsDependencyLoaded(object, dependency->name))
			continue;

		auto dependencyResult = elfObjectLoadDependency(object, dependency->name, result.nextFreeBase);
		result.nextFreeBase = dependencyResult.nextFreeBase;

		if(dependencyResult.status != G_SPAWN_STATUS_SUCCESSFUL)
		{
			result.status = dependencyResult.status;
			logInfo("%!   -> failed to load dependency %s with status %i", "elf", dependency->name,
			        dependencyResult.status);
			break;
		}
	}

	return result;
}

static uint32_t _elfObjectSymbolHash(const char* name)
{
	uint32_t hash = 0;
	while(*name)
	{
		hash = (hash << 4) + (uint8_t) *name++;
		uint32_t high = hash & 0xf0000000;
		if(high)
			hash ^= high >> 24;
		hash &= ~high;
	}
	return hash;
}

static Elf64_Sym* _elfObjectFindSymbol(g_elf_object* object, const char* name, uint32_t hash,
                                     bool lastDefinition)
{
	if(!object->dynamicSymbolHashTable || !object->dynamicSymbolTable || !object->dynamicStringTable)
		return nullptr;
	const uint32_t bucketCount = object->dynamicSymbolHashTable[0];
	if(!bucketCount)
		return nullptr;
	auto buckets = object->dynamicSymbolHashTable + 2;
	auto chains = buckets + bucketCount;
	uint32_t index = buckets[hash % bucketCount];
	uint32_t found = 0;
	// Bound traversal even when an ELF has a corrupt chain or symbol index.
	for(uint32_t remaining = object->dynamicSymbolTableSize;
	    index && index < object->dynamicSymbolTableSize && remaining; --remaining)
	{
		auto symbol = &object->dynamicSymbolTable[index];
		if(symbol->st_shndx && symbol->st_name < object->dynamicStringTableSize &&
		   stringEquals(object->dynamicStringTable + symbol->st_name, name) &&
		   (!found || (lastDefinition ? index > found : index < found)))
			found = index;
		index = chains[index];
	}
	return found ? &object->dynamicSymbolTable[found] : nullptr;
}

void elfObjectApplyRelocations(g_fd file, g_elf_object* object)
{
	g_elf_object* rootObject = object;
	while(rootObject->parent)
		rootObject = rootObject->parent;

	for(uint32_t p = 0; p < object->header.e_shnum * object->header.e_shentsize; p += object->header.e_shentsize)
	{
		Elf64_Shdr shdr;
		if(!filesystemReadToMemory(file, object->header.e_shoff + p, (uint8_t*) &shdr, object->header.e_shentsize))
		{
			logInfo("%! failed to read section header from file", "elf");
			break;
		}

		if(shdr.sh_type != SHT_RELA)
			continue;

		auto entry = (Elf64_Rela*) (object->baseAddress + shdr.sh_addr);
		while(entry < (Elf64_Rela*) (object->baseAddress + shdr.sh_addr + shdr.sh_size))
		{
			uint32_t symbolIndex = ELF64_R_SYM(entry->r_info);
			uint8_t type = ELF64_R_TYPE(entry->r_info);

			g_address cS;
			g_address cP = object->baseAddress + entry->r_offset;

			Elf64_Word symbolSize;
			const char* symbolName = nullptr;
			g_elf_symbol_info symbolInfo{};

			// Symbol lookup
			if(type == R_X86_64_32 || type == R_X86_64_64 || type == R_X86_64_PC32 ||
			   type == R_X86_64_GLOB_DAT || type == R_X86_64_JUMP_SLOT ||
			   type == R_X86_64_GOT32 || type == R_X86_64_TPOFF64 ||
			   type == R_X86_64_DTPMOD64 || type == R_X86_64_DTPOFF32 || type == R_X86_64_DTPOFF64 ||
			   type == R_X86_64_COPY || type == R_X86_64_TPOFF32)
			{
				Elf64_Sym* symbol = &object->dynamicSymbolTable[symbolIndex];
				symbolName = &object->dynamicStringTable[symbol->st_name];
				symbolSize = symbol->st_size;

				const bool copy = type == R_X86_64_COPY;
				const uint32_t hash = _elfObjectSymbolHash(symbolName);
				bool symbolFound = false;
				for(auto candidate = copy ? rootObject->symbolLookupOrderList : rootObject;
				    candidate;
				    candidate = copy ? candidate->symbolLookupOrderListNext : candidate->symbolLoadOrderNext)
				{
					auto definition = _elfObjectFindSymbol(candidate, symbolName, hash, copy);
					if(definition)
					{
						symbolInfo.object = candidate;
						symbolInfo.absolute = candidate->baseAddress + definition->st_value;
						symbolInfo.value = definition->st_value;
						symbolFound = true;
						break;
					}
				}
				if(symbolFound)
				{
					cS = symbolInfo.absolute;
				}
				else
				{
					if(ELF64_ST_BIND(symbol->st_info) != STB_WEAK)
						logDebug("%!     missing symbol '%s' (%h, bind: %i)", "elf", symbolName, cP,
					         ELF64_ST_BIND(symbol->st_info));

					cS = 0;
				}
			}

			if(type == R_X86_64_32)
			{
				int32_t cA = *((int32_t*) cP);
				*((uint32_t*) cP) = (uint32_t) (cS + cA);
			}
			else if(type == R_X86_64_PC32)
			{
				int32_t cA = *((int32_t*) cP);
				*((uint32_t*) cP) = (uint32_t) ((uint64_t) cS + cA - (uint64_t) cP);
			}
			else if(type == R_X86_64_COPY)
			{
				if(cS)
					memoryCopy((void*) cP, (void*) cS, symbolSize);
			}
			else if(type == R_X86_64_GLOB_DAT || type == R_X86_64_JUMP_SLOT)
			{
				*((uint64_t*) cP) = cS;
			}
			else if(type == R_X86_64_RELATIVE)
			{
				uint64_t cB = object->baseAddress;
				int64_t cA = *((int64_t*) cP);
				*((uint64_t*) cP) = cB + cA;
			}
			else if(type == R_X86_64_64)
			{
				int64_t cA = *((int64_t*) cP);
				*((uint64_t*) cP) = cS + cA;
			}
			else if(type == R_X86_64_TPOFF64)
			{
				if(cS)
					*((uint64_t*) cP) = symbolInfo.object->tlsPart.offset - rootObject->tlsMaster.userThreadOffset +
					                    symbolInfo.value;
			}
			else if(type == R_X86_64_TPOFF32)
			{
				if(cS)
					*((uint32_t*) cP) = (uint32_t) (
						symbolInfo.object->tlsPart.offset - rootObject->tlsMaster.userThreadOffset +
						symbolInfo.value);
			}
			else if(type == R_X86_64_DTPMOD64)
			{
				if(cS)
					*((uint64_t*) cP) = symbolInfo.object->id;
			}
			else if(type == R_X86_64_DTPOFF64)
			{
				if(cS)
					*((uint64_t*) cP) = symbolInfo.object->tlsPart.offset - rootObject->tlsMaster.userThreadOffset +
					                    symbolInfo.value;
			}
			else if(type == R_X86_64_DTPOFF32)
			{
				if(cS)
					*((uint32_t*) cP) = (uint32_t) (
						symbolInfo.object->tlsPart.offset - rootObject->tlsMaster.userThreadOffset +
						symbolInfo.value);
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

	while(elfObject->dependencies)
	{
		auto dependency = elfObject->dependencies;
		elfObject->dependencies = dependency->next;
		heapFree(dependency->name);
		heapFree(dependency);
	}
	if(elfObject->tlsPart.content)
		heapFree(elfObject->tlsPart.content);
	heapFree(elfObject->name);
	heapFree(elfObject);
}

bool elfObjectIsDependencyLoaded(g_elf_object* object, const char* name)
{
	g_elf_object* rootObject = object;
	while(rootObject->parent)
		rootObject = rootObject->parent;

	return hashmapGet(rootObject->loadedObjects, name, (g_elf_object*) nullptr) != nullptr;
}

g_elf_object_load_result elfObjectLoadDependency(g_elf_object* parentObject, const char* name, g_virtual_address base)
{
	g_fd fd = elfObjectOpenDependency(name);
	if(fd == G_FD_NONE)
		return {status: G_SPAWN_STATUS_IO_ERROR};

	return elfObjectLoad(parentObject, name, fd, base);
}

g_fd elfObjectOpenDependency(const char* name)
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
	g_fs_open_status openStatus = filesystemOpenNodeFd(findRes.node, G_FILE_FLAG_MODE_BINARY | G_FILE_FLAG_MODE_READ,
	                                                   taskingGetCurrentTask()->process->id, &fd);
	if(openStatus != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! unable to open dependency %s", "elf", absolutePath);
		heapFree(absolutePath);
		return G_FD_NONE;
	}

	heapFree(absolutePath);
	return fd;
}
