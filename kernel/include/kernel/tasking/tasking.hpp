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
#include "kernel/calls/syscall.hpp"

struct g_process;
struct g_task;

typedef bool(*g_wait_resolver)(g_task*);

/**
 * A task is a single thread executing either in user or kernel level.
 *
 * When the task is a kernel-level task, the kernelStack information will be empty.
 */
struct g_task
{
	g_process* process;
	g_tid id;
	g_security_level securityLevel;
	g_thread_status status;

	struct
	{
		g_virtual_address userThreadObject;
		g_virtual_address location;
	} tlsCopy;

	/**
	 * Pointer to the top of the stack of where the registers of this task were pushed
	 * during interruption. This may only be accessed when we are within the process
	 * address space.
	 */
	g_processor_state* state;

	struct
	{
		g_virtual_address start;
		g_virtual_address end;
	} interruptStack;

	struct
	{
		g_virtual_address start;
		g_virtual_address end;
	} stack;

	/**
	 * When the task issues a long-running syscall, a kernel task is created to execute it.
	 * In the executing task, the syscallTask field is filled with the task that processes the
	 * syscall. In the processing task, the syscallSourceTask is the task that issued the syscall.
	 */
	struct {
		g_task* processingTask;
		g_task* sourceTask;
		g_syscall_handler handler;
		void* data;
	} syscall;

	/**
	 * Wait resolver is used when a syscall must wait for something, but a kernel task
	 * would be too much overhead. The wait data is used by the resolver.
	 */
	g_wait_resolver waitResolver;
	void* waitData;
};

/**
 * Task entry used in the task list.
 */
struct g_task_entry
{
	g_task* task;
	g_task_entry* next;
};

/**
 * Processor local tasking structure. For each processor there is one instance
 * of this struct that contains the current state.
 */
struct g_tasking_local
{
	g_mutex lock;

	/**
	 * Tasking information.
	 */
	g_task_entry* list;
	g_task* current;
	int taskCount;

	/**
	 * The number of locks that are currently held on this processor. If this
	 * number is greater than 0, the scheduler will re-schedule the current task
	 * until all locks are resolved.
	 */
	int locksHeld;

	/**
	 * Approximation of milliseconds that this processor has run.
	 */
	uint32_t time;

	g_task* idleTask;
};

/**
 * Constants used as flags on virtual ranges of processes
 */
#define G_PROC_VIRTUAL_RANGE_FLAG_NONE						0
#define G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER			1

/**
 * A process groups multiple tasks.
 */
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
 * Creates an empty process. Creates a new page directory with the kernel areas
 * correctly mapped and instantiates a virtual range allocator.
 *
 * Doesn't create a thread.
 */
g_process* taskingCreateProcess();

/**
 * Creates a task.
 */
g_task* taskingCreateThread(g_virtual_address entry, g_process* process, g_security_level level);

/**
 * Adds a task to the list of scheduled tasks on the given local.
 */
void taskingAssign(g_tasking_local* local, g_task* task);

/**
 * Schedules and sets the next task as the current.
 */
void taskingSchedule();

/**
 * Sets the given task as the current task.
 */
void taskingScheduleTo(g_task* task);

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
 * Returns the processor-local tasking structure.
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

/**
 * Applies a security level on the register state of a task.
 */
void taskingApplySecurityLevel(g_processor_state* state, g_security_level securityLevel);

/**
 * Clones the TLS master from the process.
 */
void taskingPrepareThreadLocalStorage(g_task* thread);

/**
 * Yields control in a kernel task. This can only be called while no mutexes
 * are currently acquired by this thread, otherwise the kernel could get deadlocked.
 */
void taskingKernelThreadYield();

void taskingKernelThreadExit();

/**
 * Used to initialize or reset the state of a task.
 */
void taskingResetTaskState(g_task* task);

#endif
