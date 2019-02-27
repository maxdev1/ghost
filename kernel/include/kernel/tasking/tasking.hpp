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
#include "shared/system/mutex.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/memory/address_range_pool.hpp"

struct g_process;

struct g_task
{
	g_process* process;
	g_tid id;
	g_security_level securityLevel;

	g_processor_state state;
	g_virtual_address mainStack0;
	// only for user-level tasks
	g_virtual_address kernelStack0;

	struct
	{
		g_virtual_address userThreadObject;
		g_virtual_address location;
	} tlsCopy;
	;
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

	g_virtual_address kernelStackBase;
};

/**
 * Constants used as flags on virtual ranges of processes
 */
#define G_PROC_VIRTUAL_RANGE_FLAG_NONE						0
#define G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER			1

struct g_process
{
	g_mutex lock;

	g_task* main;
	g_task_entry* tasks;

	g_physical_address pageDirectory;
	g_address_range_pool* virtualRangePool;

	struct
	{
		g_virtual_address location;
		g_virtual_address copysize;
		g_virtual_address totalsize;
		g_virtual_address alignment;
	} tlsMaster;

	struct
	{
		g_virtual_address start;
		g_virtual_address end;
	} image;
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
g_process* taskingCreateProcess();

/**
 *
 */
g_task* taskingCreateThread(g_virtual_address entry, g_process* process, g_security_level level);

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
bool taskingStore(g_virtual_address esp);

/**
 * Restores the state from the current task.
 */
g_virtual_address taskingRestore(g_virtual_address esp);

/**
 *
 */
g_tasking_local* taskingGetLocal();

/**
 * Returns the next assignable task id.
 */
g_tid taskingGetNextId();

/**
 * Creates a new page directory to use for a new process. Clones the kernel space
 * into the page directory, maps the lower memory and adds recursive mapping.
 *
 * @return the physical address of the directory
 */
g_physical_address taskingCreatePageDirectory();

void taskingApplySecurityLevel(g_processor_state* state, g_security_level securityLevel);

/**
 * Clones the TLS master from the process.
 */
void taskingPrepareThreadLocalStorage(g_task* thread);

#endif
