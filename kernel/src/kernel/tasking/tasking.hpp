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

#include "kernel/tasking/task.hpp"
#include "kernel/utils/hashmap.hpp"
#include <ghost/kernel.h>
#include <ghost/system.h>

extern g_hashmap<g_tid, g_task*>* taskGlobalMap;

struct g_schedule_entry
{
	g_task* task;
	g_schedule_entry* next;
};

/**
 * Processor local tasking structure. For each processor there is one instance
 * of this struct that contains the current state.
 */
struct g_tasking_local
{
	g_mutex lock;
	uint32_t processor;

	/**
	 * When mutexes are used, each first acquire call to a mutex increases
	 * the lockCount by one, each last release call to a mutex decreases it.
	 * On the first acquire, the interrupt flag is stored and interrupts are
	 * disabled, on the last release the interrupt flag is restored.
	 */
	int lockCount;
	bool lockSetIF;

	/**
	 * Scheduling information for this processor.
	 */
	struct
	{
		g_schedule_entry* list;
		g_task* current;

		g_task* idleTask;
	} scheduling;
};

/**
 * @return the processor-local tasking structure
 */
g_tasking_local* taskingGetLocal();

/**
 * @return the task that is on this processor currently running or was
 * last running when called from within a system call handler
 */
g_task* taskingGetCurrentTask();

/**
 * Sets the currently executed task.
 */
void taskingSetCurrentTask(g_task* task);

/**
 * @return the next assignable task id
 */
g_tid taskingGetNextId();

/**
 * Initializes basic task management and necessary structures.
 */
void taskingInitializeBsp();

/**
 * Initializes the local task management for this core.
 */
void taskingInitializeAp();

/**
 * Initializes the processor-local tasking structure.
 */
void taskingInitializeLocal();

/**
 * Adds a task to the list of scheduled tasks on the given local.
 */
void taskingAssign(g_tasking_local* local, g_task* task);

/**
 * Assignes the task to the processor with the lowest load.
 */
void taskingAssignBalanced(g_task* task);

/**
 * Creates an empty process. Creates a new page directory with the kernel areas
 * correctly mapped and instantiates a virtual range allocator.
 *
 * Doesn't create a thread.
 */
g_process* taskingCreateProcess();

/**
 * Creates a task that starts execution on the given entry. The task is added to the
 * task list of the specified process. The task is scheduled only after using <taskingAssign>.
 *
 * @param entry
 * 		execution entry of the thread
 * @param process
 * 		parent process
 * @param level
 * 		security level to apply for the thread
 * @return the task or null
 */
g_task* taskingCreateTask(g_virtual_address entry, g_process* process, g_security_level level);

/**
 * Creates a special kind of task that performs a virtual 8086 call.
 */
g_task* taskingCreateTaskVm86(g_process* process, uint32_t intr, g_vm86_registers in, g_vm86_registers* out);

/**
 * Removes a thread. Cleaning up all allocated data where possible.
 */
void taskingDestroyTask(g_task* task);

/**
 * Removes a process. Cleaning up all allocated data where possible.
 */
void taskingDestroyProcess(g_process* process);

/**
 * Kills all tasks of a process.
 */
void taskingProcessKillAllTasks(g_pid pid);

/**
 * Adds the task to the process task list.
 */
void taskingProcessAddToTaskList(g_process* process, g_task* task);

/**
 * Removes the task from its process task list.
 */
void taskingProcessRemoveFromTaskList(g_task* task);

/**
 * Schedules and sets the next task as the current. This may only be called
 * during interrupt handling!
 */
void taskingSchedule();

/**
 * Stores the registers from the given state pointer (pointing to the top of the
 * kernel stack) to the state structure of the current task.
 *
 * If there is no current task (because we just initialized the system) then it
 * switches to the first task and returns false.
 */
bool taskingStore(g_virtual_address esp);

/**
 * Applies the context switch to the task which is the current one for this core. This sets
 * the correct page directory and TLS variables.
 */
void taskingApplySwitch();

/**
 * Yields control to the next task. This can only be called while no mutexes
 * are currently acquired by this thread, otherwise the kernel could get deadlocked.
 */
void taskingYield();

/**
 * Exits the current task. Sets the status to dead and yields.
 */
void taskingExit();

/**
 * Kernel thread, used by the scheduler when it should idle.
 */
void taskingIdleThread();

/**
 * Finds a task by its id.
 */
g_task* taskingGetById(g_tid id);

/**
 * When a task needs to do work within the address space of another task, it can temporarily
 * switch to that tasks directory. The override page directory of the currently executing task
 * is set to the temporary directory until using <taskingTemporarySwitchBack>.
 */
g_physical_address taskingTemporarySwitchToSpace(g_physical_address pageDirectory);

/**
 * Switches back from temporary space switch.
 */
void taskingTemporarySwitchBack(g_physical_address pageDirectory);

/**
 * Spawns an executable. This calls the correct binary loader in the background and creates a new
 * process, loading the executable object and necessary libraries. The created main thread is waiting.
 *
 * @param spawner
 * 		task calling the execution
 * @param file
 * 		executable file descriptor
 * @param securityLevel
 * 		security level of the created process
 * @param outProcess
 * 		out parameter for created process
 * @param outValidationDetails
 * 		out parameter for executable validation details
 */
g_spawn_status taskingSpawn(g_task* spawner, g_fd file, g_security_level securityLevel,
							g_process** outProcess, g_spawn_validation_details* outValidationDetails = 0);

/**
 * Waits until the task exits and then wakes the waiting task.
 */
void taskingWaitForExit(g_tid task, g_tid waiter);

#endif
