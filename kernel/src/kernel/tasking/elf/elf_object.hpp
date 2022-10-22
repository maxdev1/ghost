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

#ifndef __KERNEL_TASKING_ELF_OBJECT__
#define __KERNEL_TASKING_ELF_OBJECT__

#include "elf.h"
#include "kernel/tasking/task.hpp"
#include "kernel/utils/hashmap.hpp"
#include "kernel/utils/hashmap_string.hpp"

/**
 * Dependency structure.
 */
struct g_elf_object;
struct g_elf_dependency
{
	char* name;
	g_elf_dependency* next;
};

struct g_elf_symbol_info
{
	g_elf_object* object;
	g_virtual_address absolute;
	g_virtual_address value;
};

/**
 * Structure of an ELF32 object in memory.
 */
struct g_elf_object
{
	uint16_t id;
	g_elf_object* parent;

	bool root;
	char* name;

	Elf32_Ehdr header;
	g_elf_dependency* dependencies;

	g_virtual_address startAddress;
	g_virtual_address endAddress;
	g_virtual_address baseAddress;

	// Information about the TLS data this object has and where it should
	// be placed in the process TLS master image.
	struct
	{
		uint8_t* content;
		g_virtual_address copysize;
		g_virtual_address totalsize;
		g_virtual_address alignment;

		// Offset of this objects TLS content within the TLS master image
		// that is created for the whole executable.
		uint32_t offset;
	} tlsPart;

	// Information about the required total size of the TLS master image
	// and the user-thread offset within the TLS. Only present in the root.
	struct
	{
		uint32_t totalSize;
		uint32_t userThreadOffset;
	} tlsMaster;

	g_hashmap<const char*, g_elf_symbol_info>* localSymbols;

	// Global symbols are filled in load order and only put if not existing yet
	g_hashmap<const char*, g_elf_symbol_info>* globalSymbols;
	g_hashmap<const char*, g_elf_object*>* loadedObjects;
	uint16_t nextObjectId;

	// List ordered by symbol lookup priority, the list exists only in the
	// root object while the next-pointer exists for every object
	g_elf_object* symbolLookupOrderList;
	g_elf_object* symbolLookupOrderListNext;

	// In-address-space memory pointers
	Elf32_Dyn* dynamicSection;
	const char* dynamicStringTable;
	Elf32_Word dynamicStringTableSize;
	Elf32_Sym* dynamicSymbolTable;
	Elf32_Word dynamicSymbolTableSize;
	Elf32_Word* dynamicSymbolHashTable;

	// Initialization and destruction information
	void (*init)();
	void (*fini)();
	void (**preinitArray)(void);
	void (**initArray)(void);
	void (**finiArray)(void);
	uint16_t preinitArraySize;
	uint16_t initArraySize;
	uint16_t finiArraySize;
};

struct g_elf_object_load_result
{
	g_spawn_status status;
	g_spawn_validation_details validation;
	g_elf_object* object;
	g_virtual_address nextFreeBase;
};

/**
 * Loads an object to the current address space.
 */
g_elf_object_load_result elfObjectLoad(g_task* caller, g_elf_object* parentObject, const char* name,
									 g_fd file, g_virtual_address baseAddress, g_address_range_pool* rangeAllocator);


struct g_elf_object_load_segment_result
{
	g_spawn_status status;
	g_virtual_address alignedStart;
	g_virtual_address alignedEnd;
};

/**
 * Loads a PT_LOAD segment using the header information, relative to the base address.
 */
g_elf_object_load_segment_result elfObjectLoadLoadSegment(g_task* caller, g_fd file, Elf32_Phdr phdr, g_virtual_address baseAddress);

/**
 * Applies relocations on the given object.
 */
void elfObjectApplyRelocations(g_task* caller, g_fd file, g_elf_object* object);

/**
 * Reads information provided in the ELF object.
 */
void elfObjectInspect(g_elf_object* elfObject);

/**
 * Destroys the ELF object structure.
 */
void elfObjectDestroy(g_elf_object* elfObject);

/**
 * Loads all dependencies of an object.
 */
g_elf_object_load_result elfObjectLoadDependencies(g_task* caller, g_elf_object* parentObject, g_address_range_pool* rangeAllocator);

/**
 * Checks whether a dependency is already loaded in this context.
 */
bool elfObjectIsDependencyLoaded(g_elf_object* object, const char* name);

/**
 * Loads a shared library.
 */
g_elf_object_load_result elfObjectLoadDependency(g_task* caller, g_elf_object* parentObject, const char* name,
											   g_virtual_address baseAddress, g_address_range_pool* rangeAllocator);

/**
 * Searches for a library file and opens it.
 */
g_fd elfObjectOpenDependency(g_task* caller, const char* name);

#endif
