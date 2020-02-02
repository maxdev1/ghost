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

#include "ghost/signal.h"
#include "ghost/system.h"
#include "ghost/kernel.h"

#include "kernel/system/processor/processor.hpp"
#include "shared/system/mutex.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/memory/address_range_pool.hpp"
#include "kernel/calls/syscall.hpp"

struct g_process;
struct g_task;
struct g_tasking_local;

typedef bool (*g_wait_resolver)(g_task*);

/**
 *
 */
typedef uint8_t g_task_interruption_info_type;

#define G_TASK_INTERRUPT_INFO_TYPE_NONE		((g_task_interruption_info_type) 0)
#define G_TASK_INTERRUPT_INFO_TYPE_IRQ		((g_task_interruption_info_type) 1)
#define G_TASK_INTERRUPT_INFO_TYPE_SIGNAL	((g_task_interruption_info_type) 2)

/**
 *
 */
class g_task_interruption_info
{
public:
	g_processor_state state;
	g_processor_state* statePtr;

	g_thread_status previousStatus;
	void* previousWaitData;
	g_wait_resolver previousWaitResolver;
};

/**
 * Data used by virtual 8086 processes
 */
struct g_task_information_vm86
{
	uint8_t cpuIf;
	g_vm86_registers* out;
	uint32_t interruptRecursionLevel;
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
	 * Pointer to the processor-local tasking structure that this task is currently scheduled on.
	 */
	g_tasking_local* assignment;

	/**
	 * Number of times this task was ever scheduled.
	 */
	int timesScheduled;

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
	 * For all syscalls (threaded and non-threaded) the handler and data field are filled.
	 *
	 * When the task issues a long-running syscall, a kernel task is created to execute it.
	 * In the executing task, the processingTask field is filled with the task that processes the
	 * syscall. In the processing task, the sourceTask is the task that issued the syscall.
	 */
	struct
	{
		g_syscall_handler handler;
		void* data;

		g_task* processingTask;
		g_task* sourceTask;
	} syscall;

	/**
	 * Wait resolver is used when a syscall must wait for something, but a kernel task
	 * would be too much overhead. The wait data is used by the resolver.
	 */
	g_wait_resolver waitResolver;
	void* waitData;

	/**
	 * If the task gets interrupted by a signal or an IRQ, the current state is stored in this
	 * structure and later restored from it.
	 */
	g_task_interruption_info* interruptionInfo;

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

	/**
	 * Tasking information.
	 */
	struct
	{
		g_schedule_entry* list;
		g_task* current;
		int taskCount;

		uint32_t round;
		g_task* idleTask;
		g_task* preferredNextTask;
	} scheduling;

	/**
	 * The number of locks that are currently held on this processor.
	 * While locksHeld > 0 interrupts are disabled.
	 */
	int locksHeld;
	int locksReenableInt;
	bool inInterruptHandler;

	/**
	 * Approximation of milliseconds that this processor has run.
	 */
	uint32_t time;

};

/**
 * A signal handler registration.
 */
struct g_signal_handler
{
public:
	g_virtual_address handlerAddress = 0;
	g_virtual_address returnAddress = 0;
	g_tid task = 0;
};

/**
 * Flags used when allocating virtual ranges.
 */
#define G_PROC_VIRTUAL_RANGE_FLAG_NONE		0
/* Weak flag signals that the physical memory mapped behind the
virtual range is not managed by the kernel (for example MMIO). */
#define G_PROC_VIRTUAL_RANGE_FLAG_WEAK		1

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

	g_signal_handler signalHandlers[SIG_COUNT];

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
 * during interrupt handling! If you want to schedule a task as the next one,
 * use taskingPleaseSchedule below.
 */
void taskingSchedule();

/**
 * Sets the given task as the next one the scheduler should try to use.
 */
void taskingPleaseSchedule(g_task* task);

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
 * Yields control in a kernel task. This can only be called while no mutexes
 * are currently acquired by this thread, otherwise the kernel could get deadlocked.
 */
void taskingKernelThreadYield();

/**
 * Exits a kernel task. Sets the status of the caller task to dead and yields.
 */
void taskingKernelThreadExit();

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
 * Raises a signal in the task.
 */
g_raise_signal_status taskingRaiseSignal(g_task* task, int signal);

/**
 * Interrupts a task and continues its execution at entry. The given variadic arguments are passed to the
 * entry function. The returnAddress must point to a function that can be executed in the tasks space
 * and is called once the entry function exits.
 */
void taskingInterruptTask(g_task* task, g_virtual_address entry, g_virtual_address returnAddress, int argumentCount, ...);

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
