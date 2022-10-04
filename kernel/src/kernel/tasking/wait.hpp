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

#ifndef __KERNEL_WAIT__
#define __KERNEL_WAIT__

#include "ghost/types.h"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/tasking/tasking.hpp"

/**
 * Called by the file system if a task needs to wait until it can read from/write to a file.
 */
void waitForFile(g_task* task, g_fs_node* file, bool (*waitResolverFromDelegate)(g_fs_virt_id));

/**
 * Puts the given task into a waiting state, waiting until the other task has finished work.
 */
void waitJoinTask(g_task* task, g_tid otherTaskId);

#endif
