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

#ifndef __KERNEL_TASKING_DIRECTORY__
#define __KERNEL_TASKING_DIRECTORY__

#include "kernel/tasking/tasking.hpp"

struct g_task_directory_entry
{
    g_tid task;
    g_security_level priority;
};

/**
 * Initializes the task directory.
 */
void taskingDirectoryInitialize();

/**
 * Registers the given task in the directory. The priority decides if registering is valid,
 * a task with a stronger security level always overrides weaker, and weaker can't override
 * stronger entries.
 */
bool taskingDirectoryRegister(const char* name, g_tid tid, g_security_level priority);

/**
 * @return task for the name or G_TID_NONE
 */
g_tid taskingDirectoryGet(const char* name);

#endif
