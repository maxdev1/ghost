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
#include "ghost/system.h"

#include "kernel/calls/syscall.hpp"
#include "kernel/memory/address_range_pool.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/system/processor/processor.hpp"
#include "shared/system/mutex.hpp"

struct g_process;
struct g_task;
struct g_tasking_local;
struct g_elf_object;

/**
 * Data used by virtual 8086 processes
 */
struct g_task_information_vm86
{
	uint8_t cpuIf;
	g_vm86_registers* out;
	uint32_t interruptRecursionLevel;
	g_virtual_address userStack;
};

struct g_stack
{
	g_virtual_address start;
	g_virtual_address end;
};

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
	g_thread_type type;

	/**
	 * Indicates whether this task is currently running on the processor that its assigned to.
	 */
	g_bool active;

	/**
	 * Pointer to the processor-local tasking structure that this task is currently scheduled on.
	 */
	g_tasking_local* assignment;

	/**
	 * Number of times this task was ever scheduled.
	 */
	int timesScheduled;
	int timesYielded;

	/**
	 * Sometimes a task needs to do work in the address space of a different process.
	 * If the override page directory is set, it switches here instead of the current
	 * process directory.
	 */
	g_physical_address overridePageDirectory;

	struct
	{
		g_virtual_address userThreadObject;
		g_virtual_address start;
		g_virtual_address end;
	} tlsCopy;

	/**
	 * Pointer to the top of the stack of where the registers of this task were pushed
	 * during interruption. This may only be accessed when we are within the process
	 * address space.
	 */
	volatile g_processor_state* state;

	/**
	 * For Ring 3 tasks, the interrupt stack is used during interrupt handling.
	 */
	g_stack interruptStack;

	/**
	 * The stack that the task normally operates on.
	 */
	g_stack stack;

	/**
	 * If the thread is user-created, we must store info on where the thread should enter.
	 */
	struct
	{
		void* function;
		void* data;
	} userEntry;

	/**
	 * Only filled for VM86 tasks.
	 */
	g_task_information_vm86* vm86Data;
};

/**
 * Task entry used in the task list.
 */
struct g_task_entry
{
	g_task* task;
	g_task_entry* next;
};

struct g_schedule_entry
{
	g_task* task;
	uint32_t schedulerRound;
	g_schedule_entry* next;
};

/**
 * Processor local tasking structure. For each processor there is one instance
 * of this struct that contains the current state.
 */
struct g_tasking_local
{
	g_mutex lock;
	int lockCount;

	/**
	 * Tasking information.
	 */
	struct
	{
		g_schedule_entry* list;
		g_task* current;

		uint32_t round;
		g_task* idleTask;
	} scheduling;

	/**
	 * Approximation of milliseconds that this processor has run.
	 */
	uint64_t time;
};

/**
 * Flags used when allocating virtual ranges.
 */
#define G_PROC_VIRTUAL_RANGE_FLAG_NONE 0
/* Weak flag signals that the physical memory mapped behind the
virtual range is not managed by the kernel (for example MMIO). */
#define G_PROC_VIRTUAL_RANGE_FLAG_WEAK 1

/**
 * A process groups multiple tasks.
 */
struct g_process
{
	g_pid id;
	g_mutex lock;

	g_task* main;
	g_task_entry* tasks;

	g_physical_address pageDirectory;
	g_address_range_pool* virtualRangePool;

	struct
	{
		g_virtual_address location;
		uint32_t size;

		uint32_t userThreadOffset;
	} tlsMaster;

	struct
	{
		g_virtual_address start;
		g_virtual_address end;
	} image;
	g_elf_object* object;

	struct
	{
		g_virtual_address brk;
		g_virtual_address start;
		int pages;
	} heap;

	struct
	{
		const char* arguments;
		const char* executablePath;
		char* workingDirectory;
	} environment;

	g_process_info* userProcessInfo;
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
g_task* taskingCreateThread(g_virtual_address entry, g_process* process, g_security_level level);

/**
 * Creates a special kind of task that performs a virtual 8086 call.
 */
g_task* taskingCreateThreadVm86(g_process* process, uint32_t intr, g_vm86_registers in, g_vm86_registers* out);

/**
 * Applies a security level on the register state of a task.
 */
void taskingApplySecurityLevel(volatile g_processor_state* state, g_security_level securityLevel);

/**
 * Clones the TLS master from the process.
 */
void taskingPrepareThreadLocalStorage(g_task* thread);

/**
 * Removes a thread. Cleaning up all allocated data where possible.
 */
void taskingRemoveThread(g_task* task);

/**
 * Removes a process. Cleaning up all allocated data where possible.
 */
void taskingRemoveProcess(g_process* process);

/**
 * Kills all tasks of a process.
 */
void taskingKillProcess(g_pid pid);

/**
 * Used to initialize or reset the state of a task.
 */
void taskingResetTaskState(g_task* task);

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
 * Kernel thread, cleaning up dead tasks.
 */
void taskingCleanupThread();

/**
 * Kernel thread, used by the scheduler when it should idle.
 */
void taskingIdleThread();

/**
 * Finds a task by its id.
 */
g_task* taskingGetById(g_tid id);

/**
 * Temporarily switches this task to a different address space.
 */
g_physical_address taskingTemporarySwitchToSpace(g_physical_address pageDirectory);

/**
 * Switches back from temporarily switching to a different directory.
 */
void taskingTemporarySwitchBack(g_physical_address pageDirectory);

/**
 * Spawns an executable. This calls the correct binary loader in the background and creates a new
 * process, loading the executable object and necessary libraries and executing it.
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
 * Adds the task to the process task list.
 */
void taskingAddToProcessTaskList(g_process* process, g_task* task);

#endif
