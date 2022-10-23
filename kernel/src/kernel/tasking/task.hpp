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

#ifndef __KERNEL_TASK__
#define __KERNEL_TASK__

#include "kernel/memory/address_range_pool.hpp"
#include "kernel/system/processor/processor_state.hpp"
#include "shared/system/mutex.hpp"
#include <ghost/kernel.h>
#include <ghost/system.h>

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
};

/**
 * Task stack information
 */
struct g_stack
{
	g_virtual_address start;
	g_virtual_address end;
};

/**
 * Thread-local information used by the kernel
 */
struct g_kernel_threadlocal
{
	uint32_t processor;
};

struct g_wait_queue_entry;

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
	struct
	{
		int timesScheduled;
		int timesYielded;
	} statistics;

	/**
	 * Sometimes a task needs to do work in the address space of a different process.
	 * If the override page directory is set, it switches here instead of the current
	 * process directory.
	 */
	g_physical_address overridePageDirectory;

	/**
	 * Thread-local information for this task.
	 */
	struct
	{
		g_kernel_threadlocal* kernelThreadLocal;
		g_user_threadlocal* userThreadLocal;
		g_virtual_address start;
		g_virtual_address end;
	} threadLocal;

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

	/**
	 * List of tasks that wait for this task to die.
	 */
	g_wait_queue_entry* waitersJoin;
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
 * Flags used when allocating virtual ranges.
 */
#define G_PROC_VIRTUAL_RANGE_FLAG_NONE 0
/* Weak flag signals that the physical memory mapped behind the virtual range is not managed by the kernel (for example MMIO). */
#define G_PROC_VIRTUAL_RANGE_FLAG_WEAK 1

struct g_process_spawn_arguments
{
	g_fd fd;
	g_security_level securityLevel;
	g_address entry;

	g_spawn_status status;
	g_spawn_validation_details validation;
};

/**
 * On-demand mapping for a file in memory.
 */
struct g_memory_file_ondemand
{
	/**
	 * Source file descriptor and offset
	 */
	g_fd fd;
	g_offset fileOffset;

	/**
	 * Target address of the content in memory
	 */
	g_address fileStart;
	/**
	 * Size of content to be loaded from the file
	 */
	g_ptrsize fileSize;
	/**
	 * Total size of the allocated memory, content is followed by 0
	 */
	g_ptrsize memSize;

	g_memory_file_ondemand* next;
};

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

	/**
	 * Arguments for the spawning of this process.
	 */
	g_process_spawn_arguments* spawnArgs;

	/**
	 * List of tasks that wait for the result of spawning.
	 */
	g_wait_queue_entry* waitersSpawn;

	/**
	 * List of on-demand file-to-memory mappings.
	 */
	g_memory_file_ondemand* onDemandMappings;
};

#endif
