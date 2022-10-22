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

#ifndef __KERNEL_TASKING_ELF_LOADER__
#define __KERNEL_TASKING_ELF_LOADER__

#include "elf.h"
#include "kernel/tasking/elf/elf_object.hpp"

struct g_load_executable_result
{
	g_spawn_status status;
	g_spawn_validation_details validationDetails;
	g_address entry;
};

/**
 * Loads an ELF binary and creates a process/task for it.
 */
g_load_executable_result elfLoadExecutable(g_task* caller, g_fd file, g_security_level securityLevel, g_process* process);

/**
 * When an executable is loaded, a user process information structure
 * is created which provides information to the task (init, fini, etc.).
 */
g_virtual_address elfUserProcessCreateInfo(g_process* process, g_elf_object* executableObject,
										   g_virtual_address executableImageEnd, g_security_level securityLevel);

/**
 * Reads and validates an ELF header from a file.
 */
g_spawn_validation_details elfReadAndValidateHeader(g_task* caller, g_fd file, Elf32_Ehdr* headerBuffer, bool executable);

/**
 * Validates the given ELF header.
 */
g_spawn_validation_details elfValidateHeader(Elf32_Ehdr* header, bool executable);

/**
 * Reads a number of bytes from a file into a buffer.
 */
bool elfReadToMemory(g_task* caller, g_fd file, size_t offset, uint8_t* buffer, uint64_t length);

#endif
