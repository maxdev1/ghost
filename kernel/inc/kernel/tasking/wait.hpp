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
#include "kernel/tasking/tasking.hpp"
#include "kernel/filesystem/filesystem.hpp"

/**
 * Checks if this task can be woken up by calling its wait resolver. This call is done
 * from within the address space of the given task by temporarily switching there.
 * 
 * If it has finished waiting, the wait data is freed and the task set to
 * running state.
 */
bool waitTryWake(g_task* task);

/**
 * Puts the task to wait and lets it sleep for the given number of milliseconds.
 * If executed from the current task, you must yield.
 */
void waitSleep(g_task* task, uint64_t milliseconds);

/**
 * Lets the task wait until it can set an atom.
 */
void waitAtomicLock(g_task* task);

/**
 * Called by the file system if a task needs to wait until it can read from/write to a file.
 */
void waitForFile(g_task* task, g_fs_node* file, bool (*waitResolverFromDelegate)(g_task*));

/**
 * Puts the given task into a waiting state, waiting until the other task has finished work.
 * 
 * @note uses the <syscall.data> on the task directly
 */
void waitJoinTask(g_task* task, g_tid otherTask);

/**
 * Makes the task wait and attempts sending the requested message again.
 * 
 * @note uses the <syscall.data> on the task directly
 */
void waitForMessageSend(g_task* task);

/**
 * Makes the task wait and attempts receiving a message again.
 * 
 * @note uses the <syscall.data> on the task directly
 */
void waitForMessageReceive(g_task* task);

/**
 * Makes the task wait for the VM86 task and then copies the data from the <registerStore>
 * into the source tasks syscall data.
 * 
 * @note uses the <syscall.data> on the task directly
 */
void waitForVm86(g_task* task, g_task* vm86task, g_vm86_registers* registerStore);

#endif
