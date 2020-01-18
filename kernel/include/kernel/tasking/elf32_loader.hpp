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

#ifndef __KERNEL_ELF32_LOADER__
#define __KERNEL_ELF32_LOADER__

#include "ghost/elf32.h"
#include "kernel/filesystem/ramdisk.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/utils/hashmap.hpp"
#include "kernel/utils/hashmap_string.hpp"
#include "shared/utils/string.hpp"

struct g_elf_object;

struct g_elf_dependency {
	char* name;
	g_elf_dependency* next;
};

/**
 * Structure of an ELF32 object in memory.
 */
struct g_elf_object {
	g_elf_object* parent;
	char* name;

	elf32_ehdr header;
	g_elf_dependency* dependencies;

    g_virtual_address startAddress;
    g_virtual_address endAddress;
	g_virtual_address baseAddress;

	struct
	{
		g_virtual_address location;
		g_virtual_address copysize;
		g_virtual_address totalsize;
		g_virtual_address alignment;
	} tlsMaster;

	/* Only relevant for an executable object */
	g_hashmap<const char*, g_virtual_address>* symbols;
	g_hashmap<const char*, g_elf_object*>* loadedDependencies;

	/* In-address-space memory pointers */
	elf32_dyn* dynamicSection;

	char* stringTable;
	elf32_word stringTableSize;
	elf32_sym* symbolTable;
	elf32_word symbolTableSize;
	elf32_word* symbolHashTable;

	void (*init)();
	void (*fini)();
};

/**
 * Loads an ELF binary and creates a process for it.
 * 
 * @param caller
 * 		task starting this executable
 * @param securityLevel	
 * 		security level to apply
 * @param outTask
 * 		out parameter for created task
 * @param outValidationDetails
 * 		out parameter for validation status
 * @return the spawn status
 */
g_spawn_status elf32LoadExecutable(g_task* caller, g_fd file, g_security_level securityLevel,
	g_task** outTask = 0, g_spawn_validation_details* outValidationDetails = 0);

/**
 * Loads a shared library.
 * 
 * @param caller
 * 		task performing the loading
 * @param parentObject
 * 		parent ELF object
 * @param name
 * 		name of the shared library
 * @param baseAddress
 * 		address where to start loading the library
 * @param outNextBase
 * 		out parameter for next address after this library and all of its dependencies
 */
g_spawn_status elf32LoadLibrary(g_task* caller, g_elf_object* parentObject, const char* name,
	g_virtual_address baseAddress, g_address_range_pool* rangeAllocator,
	g_virtual_address* outNextBase, g_elf_object** outObject);

/**
 * Loads an object to the current address space.
 */
g_spawn_status elf32LoadObject(g_task* caller, g_elf_object* parentObject, const char* name,
	g_fd file, g_virtual_address baseAddress, g_address_range_pool* rangeAllocator,
	g_virtual_address* outNextBase, g_elf_object** outObject, g_spawn_validation_details* outValidationDetails = 0);

/**
 * Searches for a library file and opens it.
 */
g_fd elf32OpenLibrary(g_task* caller, const char* name);

/**
 * Loads all dependencies of an object.
 */
g_virtual_address elf32LoadDependencies(g_task* caller, g_elf_object* parentObject, g_address_range_pool* rangeAllocator);

/**
 * Loads a load segment to memory, must be called while within the target process address space.
 */
g_spawn_status elf32LoadLoadSegment(g_task* caller, g_fd fd, elf32_phdr* phdr, g_virtual_address baseAddress, g_elf_object* object);

/**
 * Loads the TLS master to memory, must be called while within the target process address space.
 */
g_spawn_status elf32LoadTlsMasterCopy(g_task* caller, g_fd file, elf32_phdr* header, g_elf_object* object, g_address_range_pool* rangeAllocator);

/**
 * Reads the ELF header from the file.
 */
g_spawn_validation_details elf32ReadAndValidateHeader(g_task* caller, g_fd file, elf32_ehdr* headerBuffer, bool executable);

/**
 * Validates the given ELF header.
 */
g_spawn_validation_details elf32Validate(elf32_ehdr* header, bool executable);

/**
 * Allocates an empty ELF object structure.
 */
g_elf_object* elf32AllocateObject();

/**
 * Reads information provided in the ELF object.
 */
void elf32InspectObject(g_elf_object* elfObject);

/**
 * Utility function to read data to memory.
 */
bool elf32ReadToMemory(g_task* caller, g_fd fd, size_t offset, uint8_t* buffer, uint64_t len);

#endif
