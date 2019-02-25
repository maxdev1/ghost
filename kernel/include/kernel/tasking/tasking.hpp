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

#ifndef __KERNEL_TASKING__
#define __KERNEL_TASKING__

#include "ghost/kernel.h"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/mutex.hpp"
#include "kernel/memory/paging.hpp"

struct g_task
{
	g_tid id;
	g_processor_state state;
	g_page_directory pageDirectory;
};

struct g_task_entry
{
	g_task* task;
	g_task_entry* next;
};

struct g_tasking_local
{
	g_mutex lock;
	g_task_entry* list;
	g_task* current;

	g_virtual_address kernelStack;
};

/**
 * Basic initialization of the task management.
 */
void taskingInitializeBsp();

/**
 * Initializes the local task management for this core.
 */
void taskingInitializeAp();

/**
 *
 */
g_task* taskingCreateThread(g_virtual_address entry, g_security_level level);

/**
 *
 */
void taskingAssign(g_tasking_local* local, g_task* task);

/**
 *
 */
void taskingSchedule();

/**
 * Stores the registers from the given state pointer (pointing to the top of the
 * kernel stack) to the state structure of the current task.
 * 
 * If there is no current task (because we just initialized the system) then it
 * switches to the first task.
 */
void taskingStore(g_processor_state* stateIn);

/**
 * Restores the state from the current task.
 */
void taskingRestore(g_processor_state* stateOut);

/**
 *
 */
g_tasking_local* taskingGetLocal();

#endif
