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

#ifndef __KERNEL_TASKING_MEMORY__
#define __KERNEL_TASKING_MEMORY__

#include "kernel/tasking/tasking.hpp"

#define G_TASKING_MEMORY_INTERRUPT_STACK_PAGES 2
#define G_TASKING_MEMORY_KERNEL_STACK_PAGES 2
#define G_TASKING_MEMORY_USER_STACK_PAGES 10

/**
 * Extends the heap of the task by an amount.
 */
bool taskingMemoryExtendHeap(g_task* task, int32_t amount, g_address* outAddress);

/**
 * Creates the stacks and other utility memory for a newly created task.
 */
void taskingMemoryInitialize(g_task* task);

/**
 * Initializes additional utility memory for the task, for example storage for
 * FPU registers.
 */
void taskingMemoryInitializeUtility(g_task* task);

/**
 * Creates the stacks for a newly created task.
 *
 * For a task running on kernel-level, only the main stack is created. On interrupt
 * handling the current state is always pushed on top of this stack.
 *
 * For user-space tasks, there is a user-space stack and a dedicated interrupt stack
 * that is switched to when handling interrupts.
 */
void taskingMemoryInitializeStacks(g_task* task);

/**
 * Creates and maps a stack.
 */
g_stack taskingMemoryCreateStack(g_address_range_pool* addressRangePool, uint32_t tableFlags, uint32_t pageFlags,
                                 int pages);

/**
 * Destroys stacks and utility memory of a task.
 */
void taskingMemoryDestroy(g_task* task);

/**
 * Frees the allocated stacks of the task.
 */
void taskingMemoryDestroyStacks(g_task* task);

/**
 * Destroys task utility memory.
 */
void taskingMemoryDestroyUtility(g_task* task);

/**
 * Destroys and unmaps a stack.
 */
void taskingMemoryDestroyStack(g_address_range_pool* addressRangePool, g_stack& stack);

/**
 * Creates a new paging space to use for a new process. Clones the kernel space
 * into the address space.
 *
 * @return the physical address of the PML4
 */
g_physical_address taskingMemoryCreatePageSpace();

/**
 * Destory the page directory of a process.
 */
void taskingMemoryDestroyPageSpace(g_physical_address directory);

/**
 * Initializes the tasks thread-local-storage. Creates a copy of the master TLS for this task.
 */
void taskingMemoryInitializeTls(g_task* task);

/**
 * Destroys the tasks thread-local-storage.
 */
void taskingMemoryDestroyTls(g_task* task);

/**
 * When a task needs to do work within the address space of another task, it can temporarily
 * switch to that tasks directory. This overrides the tasks address space until it is reset
 * using <taskingMemoryTemporarySwitchBack>.
 */
g_physical_address taskingMemoryTemporarySwitchTo(g_physical_address pageDirectory);

/**
 * Switches back from a temporary address space switch.
 */
void taskingMemoryTemporarySwitchBack(g_physical_address pageDirectory);

/**
 * Attempts to handle a stack-overflow by mapping the required pages.
 *
 * @returns whether extending the stack was successful
 */
bool taskingMemoryHandleStackOverflow(g_task* task, g_virtual_address accessedPage);

#endif
