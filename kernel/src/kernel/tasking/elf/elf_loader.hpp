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

#include "elf.h"
#include "kernel/filesystem/ramdisk.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/utils/hashmap.hpp"
#include "kernel/utils/hashmap_string.hpp"
#include "shared/logger/logger.hpp"
#include "shared/utils/string.hpp"

/**
 * ELF loader has detailed logging that can be enabled by hand
 */
#define ELF_LOADER_LOG_INFO 0
#if ELF_LOADER_LOG_INFO
#undef logDebug
#undef logDebugn
#define logDebug(msg...) logInfo(msg)
#define logDebugn(msg...) logInfon(msg)
#endif

/**
 * Constant that defines how many pages are at most loaded at once from an executable file.
 */
#define ELF_MAXIMUM_LOAD_PAGES_AT_ONCE 0x10

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
	g_elf_object* parent;
	char* name;
	bool executable;
	uint16_t id;

	Elf32_Ehdr header;
	g_elf_dependency* dependencies;

	g_virtual_address startAddress;
	g_virtual_address endAddress;
	g_virtual_address baseAddress;

	struct
	{
		uint8_t* content;
		g_virtual_address copysize;
		g_virtual_address totalsize;
		g_virtual_address alignment;

		/* Offset of this objects TLS content within the TLS master image
		that is created for the whole executable. */
		uint32_t offset;
	} tlsMaster;

	g_hashmap<const char*, g_elf_symbol_info>* localSymbols;

	/* The global symbols are filled in load order and only put if not existing yet. */
	g_hashmap<const char*, g_elf_symbol_info>* globalSymbols;
	g_hashmap<const char*, g_elf_object*>* loadedObjects;
	uint16_t nextObjectId;
	uint32_t tlsMasterTotalSize;
	uint32_t tlsMasterUserThreadOffset;

	/* The relocate order "first" is only filled in the executable object. */
	g_elf_object* relocateOrderFirst;
	g_elf_object* relocateOrderNext;

	/* In-address-space memory pointers */
	Elf32_Dyn* dynamicSection;
	const char* dynamicStringTable;
	Elf32_Word dynamicStringTableSize;
	Elf32_Sym* dynamicSymbolTable;
	Elf32_Word dynamicSymbolTableSize;
	Elf32_Word* dynamicSymbolHashTable;

	void (*init)();
	void (*fini)();
	void (**preinitArray)(void);
	void (**initArray)(void);
	void (**finiArray)(void);
	uint16_t preinitArraySize;
	uint16_t initArraySize;
	uint16_t finiArraySize;
};

/**
 * Loads an ELF binary and creates a process for it.
 *
 * @param caller
 * 		task starting this executable
 * @param securityLevel
 * 		security level to apply
 * @param outProcess
 * 		out parameter for created process
 * @param outDetails
 * 		out parameter for validation status
 * @return the spawn status
 */
g_spawn_status elfLoadExecutable(g_task* caller, g_fd file, g_security_level securityLevel,
								 g_process** outProcess = 0, g_spawn_validation_details* outDetails = 0);

/**
 * When an executable is loaded, a user process information structure must be created in the process
 * address space.
 *
 * @param process
 * 		target process
 * @param executableObject
 * 		executable elf object
 * @return address of the created structure
 */
g_virtual_address elfUserProcessCreateInfo(g_process* process, g_elf_object* executableObject,
										   g_virtual_address executableImageEnd, g_security_level securityLevel);

/**
 * Loads a PT_LOAD segment to memory, must be called while within the target process address space.
 *
 * @param caller
 * 		calling task
 * @param file
 * 		source file descriptor
 * @param phdr
 * 		program header in memory
 * @param baseAddress
 * 		where to load the segment
 * @param object
 * 		current object
 * @return status of segment loading
 */
g_spawn_status elfLoadLoadSegment(g_task* caller, g_fd file, Elf32_Phdr* phdr,
								  g_virtual_address baseAddress, g_elf_object* object);

/**
 * Reads and validates an ELF header from a file.
 *
 * @param caller
 * 		calling task
 * @param file
 * 		source file descriptor
 * @param headerBuffer
 * 		buffer to read header into
 * @param executable
 * 		whether an executable is validated
 * @returns validation status
 */
g_spawn_validation_details elfReadAndValidateHeader(g_task* caller, g_fd file, Elf32_Ehdr* headerBuffer, bool executable);

/**
 * Validates the given ELF header.
 *
 * @param header
 * 		to validate
 * @param executable
 * 		whether an executable is validated
 * @returns validation status
 */
g_spawn_validation_details elfValidate(Elf32_Ehdr* header, bool executable);

/**
 * Reads a number of bytes from a file into a buffer.
 *
 * @param caller
 * 		calling task
 * @param file
 * 		source file
 * @param offset
 * 		offset in the source file
 * @param buffer
 * 		buffer to read into
 * @param length
 * 		total number of bytes to read
 * @return whether reading was successful
 */
bool elfReadToMemory(g_task* caller, g_fd file, size_t offset, uint8_t* buffer, uint64_t length);

/**
 * Loads an object to the current address space.
 */
g_spawn_status elfObjectLoad(g_task* caller, g_elf_object* parentObject, const char* name,
							 g_fd file, g_virtual_address baseAddress, g_address_range_pool* rangeAllocator,
							 g_virtual_address* outNextBase, g_elf_object** outObject, g_spawn_validation_details* outValidationDetails = 0);

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
g_spawn_status elfLibraryLoad(g_task* caller, g_elf_object* parentObject, const char* name,
							  g_virtual_address baseAddress, g_address_range_pool* rangeAllocator,
							  g_virtual_address* outNextBase, g_elf_object** outObject);

/**
 * Searches for a library file and opens it.
 */
g_fd elfLibraryOpen(g_task* caller, const char* name);

/**
 * Loads all dependencies of an object.
 */
g_virtual_address elfLibraryLoadDependencies(g_task* caller, g_elf_object* parentObject, g_address_range_pool* rangeAllocator);

/**
 * Loads the TLS master for this object into a buffer. Then, the offset where this TLS data
 * will be loaded into the TLS master image is calculated and put into the object.
 */
g_spawn_status elfTlsLoadData(g_task* caller, g_fd file, Elf32_Phdr* header, g_elf_object* object, g_address_range_pool* rangeAllocator);

/**
 * Creates the TLS master image. The positions for each part of this image where already specified
 * when the TLS data is being loaded from each binary. The in-memory representation of our master
 * image looks like this:
 *
 * [Executable TLS content|g_user_threadlocal|Shared lib TLS content|Shared lib TLS content|...]
 *
 * When a new thread is created, a copy of this master image is created. The address of g_user_threadlocal
 * is then put into the GDT entry.
 */
void elf32TlsCreateMasterImage(g_task* caller, g_fd file, g_process* process, g_elf_object* executableObject);

#endif
