/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max SchlÃ¼ssel <lokoxe@gmail.com>                     *
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


/**
 * Spawns an ELF32 binary to a new process.
 */
g_spawn_status elf32Spawn(g_task* caller, g_fd file, g_security_level securityLevel, g_task** outTask = 0, g_spawn_validation_details* outValidationDetails = 0);

/**
 * Reads the ELF header from the file.
 */
g_spawn_validation_details elf32ReadAndValidateHeader(g_task* caller, g_fd file, elf32_ehdr* headerBuffer);

/**
 * Validates the given ELF header.
 */
g_spawn_validation_details elf32Validate(elf32_ehdr* header);

/**
 * Loads a binary to the address space of the target process. Temporarily switches to the address space of the target process.
 */
g_spawn_status elf32LoadBinaryToProcessSpace(g_task* caller, g_fd file, elf32_ehdr* header, g_process* targetProcess, g_security_level securityLevel);

/**
 * Loads a load segment to memory, must be called while within the target process address space.
 */
g_spawn_status elf32LoadLoadSegment(g_task* caller, g_fd fd, elf32_phdr* phdr, g_process* targetProcess);

/**
 * Loads the TLS master to memory, must be called while within the target process address space.
 */
g_spawn_status elf32LoadTlsMasterCopy(g_task* caller, g_fd file, elf32_phdr* header, g_process* targetProcess);

#endif
