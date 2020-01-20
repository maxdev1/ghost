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

#include "kernel/tasking/elf32_loader.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/memory.hpp"
#include "shared/logger/logger.hpp"

#define MAXIMUM_LOAD_PAGES_AT_ONCE 0x10

#define ELF_LOADER_LOG_INFO 1
#if ELF_LOADER_LOG_INFO
#undef logDebug
#undef logDebugn
#define logDebug(msg...) logInfo(msg)
#define logDebugn(msg...) logInfon(msg)
#endif

g_spawn_status elf32LoadExecutable(g_task* caller, g_fd fd, g_security_level securityLevel, g_task** outTask, g_spawn_validation_details* outValidationDetails)
{
	/* Create process and load binary */
	g_process* targetProcess = taskingCreateProcess();
	g_physical_address returnDirectory = taskingTemporarySwitchToSpace(targetProcess->pageDirectory);

	g_elf_object* executableObject;
	g_virtual_address executableEnd;
	g_spawn_validation_details validationDetails;
	g_spawn_status spawnStatus = elf32LoadObject(caller, 0, "main", fd, 0, targetProcess->virtualRangePool, &executableEnd, &executableObject, &validationDetails);
	elf32TlsCreateMasterImage(caller, fd, targetProcess, executableObject);

	taskingTemporarySwitchBack(returnDirectory);

	if(outValidationDetails) *outValidationDetails = validationDetails;

	if(spawnStatus != G_SPAWN_STATUS_SUCCESSFUL)
	{
		logInfo("%! failed to load binary to current address space", "elf");
		return spawnStatus;
	}

	/* Update process */	
	targetProcess->image.start = executableObject->startAddress;
	targetProcess->image.end = executableEnd;
	logDebug("%! process loaded to %h - %h", "elf", targetProcess->image.start, targetProcess->image.end);

	/* Create main thread */
	g_task* thread = taskingCreateThread(executableObject->header.e_entry, targetProcess, securityLevel);
	if(thread == 0)
	{
		logInfo("%! failed to create main thread to spawn ELF binary from ramdisk", "elf");
		return G_SPAWN_STATUS_TASKING_ERROR;
	}
	taskingAssign(taskingGetLocal(), thread);

	*outTask = thread;
	return G_SPAWN_STATUS_SUCCESSFUL;
}

g_spawn_status elf32LoadLibrary(g_task* caller, g_elf_object* parentObject, const char* name, g_virtual_address baseAddress,
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
	g_fd fd = elf32OpenLibrary(caller, name);
	if(fd == -1)
	{
		return G_SPAWN_STATUS_DEPENDENCY_ERROR;
	}
	g_spawn_status status = elf32LoadObject(caller, parentObject, name, fd, baseAddress, rangeAllocator, outNextBase, outObject);
	filesystemClose(caller->process->id, fd, true);
	return status;
}

g_spawn_status elf32LoadObject(g_task* caller, g_elf_object* parentObject, const char* name, g_fd file, g_virtual_address baseAddress,
	g_address_range_pool* rangeAllocator, g_virtual_address* outNextBase, g_elf_object** outObject, g_spawn_validation_details* outValidationDetails)
{
	g_spawn_status status = G_SPAWN_STATUS_SUCCESSFUL;

	/* Create ELF object */
	g_elf_object* object = elf32AllocateObject();
	object->name = stringDuplicate(name);
	object->parent = parentObject;
	object->baseAddress = baseAddress;
	object->executable = (parentObject == 0);
	if(object->executable)
	{
		object->loadedObjects = hashmapCreateString<g_elf_object*>(128);
		object->symbols = hashmapCreateString<g_elf_symbol_info>(128);
		object->nextObjectId = 0;
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
	
	logDebug("%! loading object '%s' (%i) to %h", "elf", name, object->id, baseAddress);

	/* Read and validate the ELF header */
	g_spawn_validation_details validationStatus = elf32ReadAndValidateHeader(caller, file, &object->header, object->executable);
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

		if(!elf32ReadToMemory(caller, file, phdrOffset, phdrBuffer, phdrLength))
		{
			logInfo("%! failed to read segment header from file %i", "elf", file);
			status = G_SPAWN_STATUS_IO_ERROR;
			break;
		}

		elf32_phdr* phdr = (elf32_phdr*) phdrBuffer;
		if(phdr->p_type == PT_LOAD)
		{
			status = elf32LoadLoadSegment(caller, file, phdr, baseAddress, object);
			if(status != G_SPAWN_STATUS_SUCCESSFUL)
			{
				logInfo("%! unable to load PT_LOAD segment from file", "elf");
				break;
			}

		} else if(phdr->p_type == PT_TLS)
		{
			status = elf32LoadTlsData(caller, file, phdr, object, rangeAllocator);
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
		elf32InspectObject(object);
		*outNextBase = elf32LoadDependencies(caller, object, rangeAllocator);
		elf32ApplyRelocations(caller, file, object);
	}

	return status;
}

g_virtual_address elf32LoadDependencies(g_task* caller, g_elf_object* object, g_address_range_pool* rangeAllocator)
{
	g_virtual_address nextBase = object->endAddress;

	g_elf_dependency* dependency = object->dependencies;
	while(dependency)
	{
		g_elf_object* dependencyObject;
		g_spawn_status status = elf32LoadLibrary(caller, object, dependency->name, nextBase, rangeAllocator, &nextBase, &dependencyObject);

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

void elf32InspectObject(g_elf_object* object) {

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

		/* Put symbols into executables dynamic symbol table */
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
				if(it->st_shndx && hashmapGetEntry<const char*, g_elf_symbol_info>(executableObject->symbols, symbol) == 0)
				{
					g_elf_symbol_info symbolInfo;
					symbolInfo.object = object;
					symbolInfo.absolute = object->baseAddress + it->st_value;
					symbolInfo.value = it->st_value;
					hashmapPut<const char*, g_elf_symbol_info>(executableObject->symbols, symbol, symbolInfo);
					// logDebug("%#     found symbol: %s, %h", symbol, object->baseAddress + it->st_value);
				}

				it++;
				pos++;
			}
		}
	}
}

g_spawn_status elf32LoadLoadSegment(g_task* caller, g_fd file, elf32_phdr* phdr, g_virtual_address baseAddress, g_elf_object* object)
{
	/* Calculate addresses where segment is loaded */
	g_virtual_address loadBase = baseAddress + phdr->p_vaddr;
	g_virtual_address loadEnd = loadBase + phdr->p_filesz;
	g_virtual_address memoryStart = loadBase & ~G_PAGE_ALIGN_MASK;
	g_virtual_address memoryEnd = ((loadBase + phdr->p_memsz) & ~G_PAGE_ALIGN_MASK) + G_PAGE_SIZE;

	if(object->startAddress == 0 || memoryStart < object->startAddress) {
		object->startAddress = memoryStart;
	}
	if(memoryEnd > object->endAddress) {
		object->endAddress = memoryEnd;
	}

	uint32_t pagesTotal = (memoryEnd - memoryStart) / G_PAGE_SIZE;
	uint32_t pagesLoaded = 0;

	uint32_t loadPosition = loadBase;
	uint32_t readOffset = phdr->p_offset;
	while(pagesLoaded < pagesTotal)
	{
		/* Allocate memory */
		uint32_t areaPages = (pagesTotal - pagesLoaded);
		if(areaPages > MAXIMUM_LOAD_PAGES_AT_ONCE)
			areaPages = MAXIMUM_LOAD_PAGES_AT_ONCE;

		g_virtual_address areaStart = memoryStart + pagesLoaded * G_PAGE_SIZE;
		g_virtual_address areaEnd = (areaStart + areaPages * G_PAGE_SIZE);

		for(uint32_t i = 0; i < areaPages; i++)
		{
			g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
			pageReferenceTrackerIncrement(page);
			pagingMapPage(areaStart + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		}

		if(loadPosition < loadEnd)
		{
			uint32_t copyBytes = 0;
			if(loadEnd < areaEnd) {
				copyBytes = loadEnd - loadPosition;
			} else {
				copyBytes = areaEnd - loadPosition;
			}

			/* Zero area before content in memory (if we are at the start) */
			uint32_t sizeBefore = loadPosition - areaStart;
			if(sizeBefore > 0)
			{
				memorySetBytes((void*) areaStart, 0, sizeBefore);

				logDebug("%!   [%h-%h] zero %h bytes before code", "elf", areaStart, areaStart + sizeBefore, sizeBefore);
			}

			/* Copy data to memory */
			if(!elf32ReadToMemory(caller, file, readOffset, (uint8_t*) loadPosition, copyBytes))
			{
				logInfo("%! unable to read data for PT_LOAD header", "elf");
				return G_SPAWN_STATUS_IO_ERROR;
			}
			logDebug("%!   [%h-%h] read %h bytes from file %h", "elf", loadPosition, loadPosition + copyBytes, copyBytes, readOffset);

			/* Zero memory after content in memory */
			uint32_t loadEnd = loadPosition + copyBytes;
			uint32_t sizeAfter = areaEnd - loadEnd;
			if(sizeAfter > 0)
			{
				memorySetBytes((void*) loadEnd, 0, sizeAfter);

				logDebug("%!   [%h-%h] zero %h bytes after code", "elf", loadEnd, loadEnd + sizeAfter, sizeAfter);
			}

			loadPosition += copyBytes;
			readOffset += copyBytes;

		} else {
			/* Zero area without any content */
			uint32_t areaSize = areaPages * G_PAGE_SIZE;
			memorySetBytes((void*) areaStart, 0, areaSize);
			loadPosition += areaSize;

			logDebug("%!   [%h-%h] zero %h bytes of blank", "elf", areaStart, areaEnd, areaSize);
		}

		pagesLoaded += areaPages;
	}

	return G_SPAWN_STATUS_SUCCESSFUL;
}

void elf32ApplyRelocations(g_task* caller, g_fd file, g_elf_object* object)
{
	logDebug("%!   applying relocations for '%s'", "elf", object->name);
	g_elf_object* executableObject = object;
	while(executableObject->parent) {
		executableObject = executableObject->parent;
	}
	
	for(uint32_t p = 0; p < object->header.e_shnum * object->header.e_shentsize; p += object->header.e_shentsize) {
		
		elf32_shdr sectionHeader;
		if(!elf32ReadToMemory(caller, file, object->header.e_shoff + p, (uint8_t*) &sectionHeader, object->header.e_shentsize)) {
			logInfo("%! failed to read section header from file", "elf");
			break;
		}

		// TODO only resolve relocations in PLT if LD_BIND_NOW is set
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
			const char* symbolName;
			g_elf_symbol_info symbolInfo;

			if (type == R_386_32 || type == R_386_PC32 || type == R_386_GLOB_DAT || type == R_386_JMP_SLOT || type == R_386_GOTOFF ||
				type == R_386_COPY || type == R_386_TLS_TPOFF || type == R_386_TLS_DTPMOD32 || type == R_386_TLS_DTPOFF32)
			{
				elf32_sym* symbol = &object->dynamicSymbolTable[symbolIndex];
				symbolName = &object->dynamicStringTable[symbol->st_name];
				symbolSize = symbol->st_size;
				
				auto entry = hashmapGetEntry<const char*, g_elf_symbol_info>(executableObject->symbols, symbolName);
				if(entry == 0) {
					logInfo("%!     missing symbol '%s' (%h, type %i)", "elf", symbolName, cP, type);
					cS = 0;
				}
				else {
					symbolInfo = entry->value;
					cS = symbolInfo.absolute;
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
				}

			} else if(type == R_386_JMP_SLOT || type == R_386_GLOB_DAT)
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

bool elf32ReadToMemory(g_task* caller, g_fd fd, size_t offset, uint8_t* buffer, uint64_t len)
{
	int64_t seeked;
	g_fs_seek_status seekStatus = filesystemSeek(caller, fd, G_FS_SEEK_SET, offset, &seeked);
	if(seekStatus != G_FS_SEEK_SUCCESSFUL)
	{
		logInfo("%! failed to seek to %i binary from fd %i", "elf", offset, fd);
		return false;
	}
	if(seeked != offset)
	{
		logInfo("%! tried to seek in file to position %i but only got to %i", "elf", (uint32_t )offset, (uint32_t )seeked);
	}

	uint64_t remain = len;
	while(remain)
	{
		int64_t read;
		g_fs_read_status readStatus = filesystemRead(caller, fd, &buffer[len - remain], remain, &read);
		if(readStatus != G_FS_READ_SUCCESSFUL)
		{
			logInfo("%! failed to read binary from fd %i", "elf", fd);
			return false;
		}
		remain -= read;
	}
	return true;
}

g_spawn_validation_details elf32ReadAndValidateHeader(g_task* caller, g_fd file, elf32_ehdr* headerBuffer, bool executable)
{
	if(!elf32ReadToMemory(caller, file, 0, (uint8_t*) headerBuffer, sizeof(elf32_ehdr)))
	{
		logInfo("%! failed to spawn file %i due to io error", "elf", file);
		return G_SPAWN_VALIDATION_ELF32_IO_ERROR;
	}
	return elf32Validate(headerBuffer, executable);
}

g_spawn_validation_details elf32Validate(elf32_ehdr* header, bool executable)
{
	if(/* */(header->e_ident[EI_MAG0] != ELFMAG0) ||      // 0x7F
			(header->e_ident[EI_MAG1] != ELFMAG1) ||	  // E
			(header->e_ident[EI_MAG2] != ELFMAG2) ||	  // L
			(header->e_ident[EI_MAG3] != ELFMAG3))		  // F
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_ELF;
	}

	/* Check executable flag */
	if(executable && header->e_type != ET_EXEC)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_EXECUTABLE;
	}

	/* Must be i386 architecture compatible */
	if(header->e_machine != EM_386)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_I386;
	}

	/* Must be 32 bit */
	if(header->e_ident[EI_CLASS] != ELFCLASS32)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_32BIT;
	}

	/* Must be little endian */
	if(header->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_LITTLE_ENDIAN;
	}

	/* Must comply to current ELF standard */
	if(header->e_version != EV_CURRENT)
	{
		return G_SPAWN_VALIDATION_ELF32_NOT_STANDARD_ELF;
	}

	return G_SPAWN_VALIDATION_SUCCESSFUL;
}

g_elf_object* elf32AllocateObject()
{
	g_elf_object* object = (g_elf_object*) heapAllocate(sizeof(g_elf_object));
	memorySetBytes((void*) object, 0, sizeof(g_elf_object));
	return object;
}

g_fd elf32OpenLibrary(g_task* caller, const char* name)
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
		logInfo("%! unable to resolve dependency %s", "elf", name);
		heapFree(absolutePath);
		return -1;
	}

	g_fd fd;
	g_fs_open_status openStatus = filesystemOpen(file, G_FILE_FLAG_MODE_BINARY | G_FILE_FLAG_MODE_READ, caller, &fd);
	if(openStatus != G_FS_OPEN_SUCCESSFUL) {
		logInfo("%! unable to open dependency %s", "elf", absolutePath);
		heapFree(absolutePath);
		return -1;
	}

	heapFree(absolutePath);
	return fd;
}


g_spawn_status elf32LoadTlsData(g_task* caller, g_fd file, elf32_phdr* phdr, g_elf_object* object, g_address_range_pool* rangeAllocator)
{
	uint32_t bytesToCopy = phdr->p_filesz;

	/* Read TLS content to a buffer */
	uint8_t* tlsContentBuffer = (uint8_t*) heapAllocate(bytesToCopy);
	if(!elf32ReadToMemory(caller, file, phdr->p_offset, (uint8_t*) tlsContentBuffer, bytesToCopy))
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
	g_virtual_address tlsStart = addressRangePoolAllocate(process->virtualRangePool, requiredPages, G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER);
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
