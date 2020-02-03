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


g_spawn_status elfObjectLoad(g_task* caller, g_elf_object* parentObject, const char* name, g_fd file, g_virtual_address baseAddress,
	g_address_range_pool* rangeAllocator, g_virtual_address* outNextBase, g_elf_object** outObject, g_spawn_validation_details* outValidationDetails)
{
	g_spawn_status status = G_SPAWN_STATUS_SUCCESSFUL;

	/* Create ELF object */
	g_elf_object* object = elfObjectAllocate();
	object->name = stringDuplicate(name);
	object->parent = parentObject;
	object->baseAddress = baseAddress;
	object->executable = (parentObject == 0);
	object->localSymbols = hashmapCreateString<g_elf_symbol_info>(16);
	if(object->executable)
	{
		object->loadedObjects = hashmapCreateString<g_elf_object*>(16);
		object->globalSymbols = hashmapCreateString<g_elf_symbol_info>(16);
		object->nextObjectId = 0;
		object->relocateOrderFirst = 0;
	}
	*outObject = object;

	/* Put in list of loaded dependencies */
	g_elf_object* executableObject = object;
	while(executableObject->parent)
	{
		executableObject = executableObject->parent;
	}
	object->id = executableObject->nextObjectId++;
	hashmapPut<const char*, g_elf_object*>(executableObject->loadedObjects, name, object);

	/* Put in relocate order list */
	object->relocateOrderNext = executableObject->relocateOrderFirst;
	executableObject->relocateOrderFirst = object;
	
	logDebug("%! loading object '%s' (%i) to %h", "elf", name, object->id, baseAddress);

	/* Read and validate the ELF header */
	g_spawn_validation_details validationStatus = elfReadAndValidateHeader(caller, file, &object->header, object->executable);
	if(outValidationDetails)
	{
		*outValidationDetails = validationStatus;
	}
	if(validationStatus != G_SPAWN_VALIDATION_SUCCESSFUL)
	{
		logInfo("%! validation failed when loading object %s with status %i", "elf", name, validationStatus);
		return G_SPAWN_STATUS_FORMAT_ERROR;
	}

	/* Load all program headers */
	for(uint32_t i = 0; i < object->header.e_phnum; i++)
	{
		uint32_t phdrOffset = object->header.e_phoff + object->header.e_phentsize * i;
		uint32_t phdrLength = sizeof(elf32_phdr);
		uint8_t phdrBuffer[phdrLength];

		if(!elfReadToMemory(caller, file, phdrOffset, phdrBuffer, phdrLength))
		{
			logInfo("%! failed to read segment header from file %i", "elf", file);
			status = G_SPAWN_STATUS_IO_ERROR;
			break;
		}

		elf32_phdr* phdr = (elf32_phdr*) phdrBuffer;
		if(phdr->p_type == PT_LOAD)
		{
			status = elfLoadLoadSegment(caller, file, phdr, baseAddress, object);
			if(status != G_SPAWN_STATUS_SUCCESSFUL)
			{
				logInfo("%! unable to load PT_LOAD segment from file", "elf");
				break;
			}

		} else if(phdr->p_type == PT_TLS)
		{
			status = elfTlsLoadData(caller, file, phdr, object, rangeAllocator);
			if(status != G_SPAWN_STATUS_SUCCESSFUL)
			{
				logInfo("%! unable to load PT_TLS segment from file", "elf");
				break;
			}

		} else if(phdr->p_type == PT_DYNAMIC)
		{ 
			object->dynamicSection = (elf32_dyn*) (baseAddress + phdr->p_vaddr);
			logDebug("%!   object has dynamic information %h", "elf", object->dynamicSection);
		}
	}

	/* Do analyzation and linking */
	if(status == G_SPAWN_STATUS_SUCCESSFUL) {
		elfObjectInspect(object);
		*outNextBase = elfLibraryLoadDependencies(caller, object, rangeAllocator);
		elfObjectApplyRelocations(caller, file, object);
	}

	return status;
}

void elfObjectInspect(g_elf_object* object) {

	if(object->dynamicSection) {
		/* Find tables that we need */
		elf32_dyn* it = object->dynamicSection;
		while(it->d_tag) {
			switch(it->d_tag) {
				case DT_STRTAB:
					object->dynamicStringTable = (char*) (object->baseAddress + it->d_un.d_ptr);
					break;
				case DT_STRSZ:
					object->dynamicStringTableSize = it->d_un.d_val;
					break;
				case DT_HASH:
					object->dynamicSymbolHashTable = (elf32_word*) (object->baseAddress + it->d_un.d_ptr);
					/*  The number of symbol table entries should equal nchain; so symbol table indexes also select chain table entries. */
					object->dynamicSymbolTableSize = object->dynamicSymbolHashTable[1]; 
					break;
				case DT_SYMTAB:
					object->dynamicSymbolTable = (elf32_sym*) (object->baseAddress + it->d_un.d_ptr);
					break;
				case DT_INIT:
					object->init = (void(*)()) (object->baseAddress + it->d_un.d_ptr);
					break;
				case DT_FINI:
					object->fini = (void(*)()) (object->baseAddress + it->d_un.d_ptr);
					break;
				case DT_PREINIT_ARRAY:
					object->preinitArray = (void(**)()) (object->baseAddress + it->d_un.d_ptr);
					break;
				case DT_PREINIT_ARRAYSZ:
					object->preinitArraySize = it->d_un.d_val / sizeof(uintptr_t);
					break;
				case DT_INIT_ARRAY:
					object->initArray = (void(**)()) (object->baseAddress + it->d_un.d_ptr);
					break;
				case DT_INIT_ARRAYSZ:
					object->initArraySize = it->d_un.d_val / sizeof(uintptr_t);
					break;
				case DT_FINI_ARRAY:
					object->finiArray = (void(**)()) (object->baseAddress + it->d_un.d_ptr);
					break;
				case DT_FINI_ARRAYSZ:
					object->finiArraySize = it->d_un.d_val / sizeof(uintptr_t);
					break;
			}
			it++;
		}

		/* Read dependencies */
		object->dependencies = 0;
		it = object->dynamicSection;
		while(it->d_tag) {
			if(it->d_tag == DT_NEEDED) {
				g_elf_dependency* dep = (g_elf_dependency*) heapAllocate(sizeof(g_elf_dependency));
				dep->name = stringDuplicate(object->dynamicStringTable + it->d_un.d_val);
				dep->next = object->dependencies;
				object->dependencies = dep;
			}
			it++;
		}

		/* Put symbols into executables dynamic symbol table. This happens in load order. */
		if(object->dynamicSymbolTable) {
			g_elf_object* executableObject = object;
			while(executableObject->parent)
			{
				executableObject = executableObject->parent;
			}

			uint32_t pos = 0;
			elf32_sym* it = object->dynamicSymbolTable;
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

					if(hashmapGetEntry<const char*, g_elf_symbol_info>(executableObject->globalSymbols, symbol) == 0)
					{
						hashmapPut<const char*, g_elf_symbol_info>(executableObject->globalSymbols, symbol, symbolInfo);
					}
				}

				it++;
				pos++;
			}
		}
	}
}

g_elf_object* elfObjectAllocate()
{
	g_elf_object* object = (g_elf_object*) heapAllocate(sizeof(g_elf_object));
	memorySetBytes((void*) object, 0, sizeof(g_elf_object));
	return object;
}
