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

#ifndef GHOST_EXECUTABLE_ELF32_LOADER
#define GHOST_EXECUTABLE_ELF32_LOADER

#include "ghost/elf32.h"
#include <ramdisk/ramdisk.hpp>
#include <memory/paging.hpp>
#include <tasking/thread.hpp>

/**
 * Executable spawn status
 */
enum g_elf32_spawn_status {
	ELF32_SPAWN_STATUS_SUCCESSFUL, ELF32_SPAWN_STATUS_FILE_NOT_FOUND, ELF32_SPAWN_STATUS_VALIDATION_ERROR, ELF32_SPAWN_STATUS_PROCESS_CREATION_FAILED
};

/**
 * ELF validation status
 */
enum g_elf32_validation_status {
	ELF32_VALIDATION_SUCCESSFUL,
	ELF32_VALIDATION_NOT_ELF,
	ELF32_VALIDATION_NOT_EXECUTABLE,
	ELF32_VALIDATION_NOT_I386,
	ELF32_VALIDATION_NOT_32BIT,
	ELF32_VALIDATION_NOT_LITTLE_ENDIAN,
	ELF32_VALIDATION_NOT_STANDARD_ELF
};

/**
 * Executable loader for 32bit ELF binaries
 */
class g_elf32_loader {
public:
	static g_elf32_spawn_status spawnFromRamdisk(g_ramdisk_entry* binaryFile, g_security_level securityLevel, g_thread** target, bool enforceCurrentCore,
			g_thread_priority priority);

private:
	static g_elf32_validation_status validate(elf32_ehdr* header);
	static void loadBinaryToCurrentAddressSpace(elf32_ehdr* binaryHeader, g_process* process);
	static void loadTlsMasterCopy(elf32_ehdr* header, g_process* process);
	static void loadLoadSegment(elf32_ehdr* header, g_process* process);
};

#endif
