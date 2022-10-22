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

#ifndef __KERNEL_TASKING_ELF_TLS__
#define __KERNEL_TASKING_ELF_TLS__

#include "elf.h"
#include "kernel/tasking/task.hpp"

/**
 * Loads the TLS master for this object into a buffer. Then, the offset where this TLS data
 * will be loaded into the TLS master image is calculated and put into the object.
 */
g_spawn_status elfTlsLoadData(g_task* caller, g_fd file, Elf32_Phdr header, g_elf_object* object);

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
void elfTlsCreateMasterImage(g_task* caller, g_fd file, g_process* process, g_elf_object* executableObject);

#endif
