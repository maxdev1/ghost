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

#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/scheduler.hpp"
#include "kernel/calls/syscall.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/gdt.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/kernel.hpp"
#include "shared/logger/logger.hpp"

g_tasking_local* taskingLocal;

static g_mutex taskingIdLock = 0;
static g_tid taskingIdNext = 0;

void taskingInitializeLocal(g_tasking_local* local)
{
	local->lock = 0;
	local->list = 0;
	local->current = 0;
	local->locksHeld = 0;
	local->time = 0;
}

void taskingInitializeBsp()
{
	taskingLocal = (g_tasking_local*) heapAllocate(sizeof(g_tasking_local) * processorGetNumberOfProcessors());

	g_tasking_local* local = &taskingLocal[processorGetCurrentId()];
	taskingInitializeLocal(local);
	
	syscallRegisterAll();
}

void taskingInitializeAp()
{
	g_tasking_local* local = &taskingLocal[processorGetCurrentId()];
	taskingInitializeLocal(local);
}

g_physical_address taskingCreatePageDirectory()
{
	g_page_directory directoryCurrent = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;

	g_physical_address directoryPhys = (g_physical_address) bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	g_virtual_address directoryVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	g_page_directory directoryTemp = (g_page_directory) directoryVirt;
	pagingMapPage(directoryVirt, directoryPhys);

	// clone kernel space mappings
	for(uint32_t ti = 0; ti < 1024; ti++)
	{
		if(!((directoryCurrent[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE))
			directoryTemp[ti] = directoryCurrent[ti];
		else
			directoryTemp[ti] = 0;
	}

	// clone mappings for the lowest 4 MiB TODO
	directoryTemp[0] = directoryCurrent[0];

	// recursive self-map
	directoryTemp[1023] = directoryPhys | DEFAULT_KERNEL_TABLE_FLAGS;

	pagingUnmapPage(directoryVirt);
	return directoryPhys;
}

void taskingApplySecurityLevel(g_processor_state* state, g_security_level securityLevel)
{
	if(securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		state->cs = G_GDT_DESCRIPTOR_KERNEL_CODE | G_SEGMENT_SELECTOR_RING0;
		state->ss = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->ds = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->es = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->fs = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->gs = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
	} else
	{
		state->cs = G_GDT_DESCRIPTOR_USER_CODE | G_SEGMENT_SELECTOR_RING3;
		state->ss = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->ds = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->es = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->fs = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->gs = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
	}

	if(securityLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		state->eflags |= 0x3000; // IOPL 3
	}
}

g_task* taskingCreateThread(g_virtual_address eip, g_process* process, g_security_level level)
{
	g_task* task = (g_task*) heapAllocate(sizeof(g_task));
	task->id = taskingGetNextId();
	task->process = process;
	task->securityLevel = level;

	task->tlsCopy.location = 0;
	task->tlsCopy.userThreadObject = 0;

	// Switch to task directory
	g_physical_address currentDir = (g_physical_address) pagingGetCurrentSpace();
	pagingSwitchToSpace(task->process->pageDirectory);

	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		// For kernel level tasks, registers are stored on working stack on interrupt
		task->interruptStack.start = 0;
		task->interruptStack.end = 0;
	} else
	{
		// User-space processes get a dedicated kernel stack
		g_physical_address kernPhys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		g_virtual_address kernVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
		pagingMapPage(kernVirt, kernPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
		task->interruptStack.start = kernVirt;
		task->interruptStack.end = kernVirt + G_PAGE_SIZE;
	}

	// Create main stack
	g_physical_address stackPhys = (g_physical_address) bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	g_virtual_address stackVirt;
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		stackVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
		pagingMapPage(stackVirt, stackPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	} else
	{
		stackVirt = addressRangePoolAllocate(process->virtualRangePool, 1);
		pagingMapPage(stackVirt, stackPhys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}
	task->stack.start = stackVirt;
	task->stack.end = stackVirt + G_PAGE_SIZE;

	// Initialize task state on main stack
	g_virtual_address esp = task->stack.end - sizeof(g_processor_state);
	task->state = (g_processor_state*) esp;

	memorySetBytes((void*) task->state, 0, sizeof(g_processor_state));
	task->state->esp = esp;
	task->state->eip = eip;
	task->state->eflags = 0x200;
	taskingApplySecurityLevel(task->state, task->securityLevel);

	// Switch back
	pagingSwitchToSpace(currentDir);

	// Add to process
	mutexAcquire(&process->lock);
	g_task_entry* entry = (g_task_entry*) heapAllocate(sizeof(g_task_entry));
	entry->task = task;
	entry->next = process->tasks;
	process->tasks = entry;
	if(process->main == 0)
	{
		process->main = task;
	}
	taskingPrepareThreadLocalStorage(task);
	mutexRelease(&process->lock);

	return task;
}

void taskingAssign(g_tasking_local* local, g_task* task)
{
	mutexAcquire(&local->lock);

	g_task_entry* newEntry = (g_task_entry*) heapAllocate(sizeof(g_task_entry));
	newEntry->task = task;
	newEntry->next = local->list;
	local->list = newEntry;

	mutexRelease(&local->lock);
}

bool taskingStore(g_virtual_address esp)
{
	g_task* task = taskingGetLocal()->current;

	// Very first interrupt that happened on this processor
	if(!task)
	{
		taskingSchedule();
		return false;
	}

	// Store where registers were pushed to
	task->state = (g_processor_state*) esp;

	return true;
}

g_virtual_address taskingRestore(g_virtual_address esp)
{
	g_task* task = taskingGetLocal()->current;
	if(!task)
		kernelPanic("%! tried to restore without a current task", "tasking");

	// Switch to process address space
	pagingSwitchToSpace(task->process->pageDirectory);

	// For TLS: write user thread address to GDT & set GS of thread to user pointer segment
	gdtSetUserThreadObjectAddress(task->tlsCopy.userThreadObject);
	task->state->gs = 0x30;

	// Set TSS ESP0 for ring 3 tasks to return onto
	gdtSetTssEsp0(task->interruptStack.end);

	return (g_virtual_address) task->state;
}

void taskingSchedule()
{
	g_tasking_local* local = taskingGetLocal();

	// If there are kernel locks held by the currently running task, we may not
	// switch tasks - otherwise we will deadlock.
	if(local->locksHeld == 0)
	{
		mutexAcquire(&local->lock);
		schedulerSchedule(local);
		mutexRelease(&local->lock);
	}
}

g_tasking_local* taskingGetLocal()
{
	return &taskingLocal[processorGetCurrentId()];
}

g_tid taskingGetNextId()
{
	mutexAcquire(&taskingIdLock);
	g_tid next = taskingIdNext++;
	mutexRelease(&taskingIdLock);
	return next;
}

g_process* taskingCreateProcess()
{
	g_process* process = (g_process*) heapAllocate(sizeof(g_process));
	process->main = 0;
	process->tasks = 0;
	process->lock = 0;

	process->tlsMaster.location = 0;
	process->tlsMaster.copysize = 0;
	process->tlsMaster.totalsize = 0;
	process->tlsMaster.alignment = 0;

	process->pageDirectory = taskingCreatePageDirectory();

	process->virtualRangePool = (g_address_range_pool*) heapAllocate(sizeof(g_address_range_pool));
	addressRangePoolInitialize(process->virtualRangePool);
	addressRangePoolAddRange(process->virtualRangePool, G_CONST_USER_VIRTUAL_RANGES_START, G_CONST_USER_VIRTUAL_RANGES_END);

	return process;
}

void taskingPrepareThreadLocalStorage(g_task* thread)
{
	// if tls master copy available, copy it to thread
	g_process* process = thread->process;
	if(process->tlsMaster.location == 0)
	{
		logDebug("%! failed to copy tls master, not available in process", "tls");
		return;
	}

	// calculate size that TLS needs including alignment
	uint32_t alignedTotalSize = G_ALIGN_UP(process->tlsMaster.totalsize, process->tlsMaster.alignment);

	// allocate virtual range with aligned size of TLS + size of {g_user_thread}
	uint32_t requiredSize = alignedTotalSize + sizeof(g_user_thread);
	uint32_t requiredPages = G_PAGE_ALIGN_UP(requiredSize) / G_PAGE_SIZE;
	g_virtual_address tlsCopyVirt = addressRangePoolAllocate(process->virtualRangePool, requiredPages, G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER);

	// store executing space
	g_physical_address currentPd = pagingGetCurrentSpace();

	// temporarily switch to target process directory, copy TLS contents
	pagingSwitchToSpace(process->pageDirectory);
	for(uint32_t i = 0; i < requiredPages; i++)
	{
		g_physical_address phys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pagingMapPage(tlsCopyVirt + i * G_PAGE_SIZE, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		pageReferenceTrackerIncrement(phys);
	}

	// zero & copy TLS content
	memorySetBytes((void*) tlsCopyVirt, 0, process->tlsMaster.totalsize);
	memoryCopy((void*) tlsCopyVirt, (void*) process->tlsMaster.location, process->tlsMaster.copysize);

	// fill user thread
	g_virtual_address userThreadObject = tlsCopyVirt + alignedTotalSize;
	g_user_thread* userThread = (g_user_thread*) userThreadObject;
	userThread->self = userThread;

	// switch back
	pagingSwitchToSpace(currentPd);

	// set threads TLS location
	thread->tlsCopy.userThreadObject = userThreadObject;
	thread->tlsCopy.location = tlsCopyVirt;

	logDebug("%! created tls copy in process %i, thread %i at %h", "threadmgr", process->main->id, thread->id, thread->tlsCopy.location);
}

void taskingKernelThreadYield() {

	g_tasking_local* local = taskingGetLocal();
	if(local->locksHeld > 0) {
		logInfo("%! warning: kernel thread %i tried to yield while holding a kernel lock", "tasking", local->current->id);
		return;
	}

	asm volatile ("int $0x81"
			:
			: "a"(0), "b"(0)
			: "cc", "memory");
}

