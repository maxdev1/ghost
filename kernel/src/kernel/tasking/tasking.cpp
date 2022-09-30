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

#include "ghost/calls/calls.h"
#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/ipc/message.hpp"
#include "kernel/kernel.hpp"
#include "kernel/memory/gdt.hpp"
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/system/interrupts/ivt.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/elf/elf_loader.hpp"
#include "kernel/tasking/scheduler.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "kernel/tasking/wait.hpp"
#include "kernel/utils/hashmap.hpp"
#include "shared/logger/logger.hpp"

static g_tasking_local* taskingLocal = 0;
static g_mutex taskingIdLock;
static g_tid taskingIdNext = 0;

static g_hashmap<g_tid, g_task*>* taskGlobalMap;

g_tasking_local* taskingGetLocal()
{
	return &taskingLocal[processorGetCurrentId()];
}

g_task* taskingGetCurrentTask()
{
	if(!systemIsReady())
		return nullptr;
	return taskingGetLocal()->scheduling.current;
}

void taskingSetCurrentTask(g_task* task)
{
	taskingGetLocal()->scheduling.current = task;
	taskingApplySwitch();
}

g_tid taskingGetNextId()
{
	mutexAcquire(&taskingIdLock);
	g_tid next = taskingIdNext++;
	mutexRelease(&taskingIdLock);
	return next;
}

g_task* taskingGetById(g_tid id)
{
	return hashmapGet(taskGlobalMap, id, (g_task*) 0);
}

/**
 * When yielding, store the state pointer (on top of the interrupt stack)
 * and when returned, restore this state.
 */
void taskingYield()
{
	if(!systemIsReady())
		return;

	g_task* task = taskingGetCurrentTask();
	task->timesYielded++;

	auto previousState = task->state;
	asm volatile("int $0x81" ::
					 : "cc", "memory");
	taskingGetCurrentTask()->state = previousState;
}

void taskingExit()
{
	taskingGetCurrentTask()->status = G_THREAD_STATUS_DEAD;
	taskingYield();
}

void taskingInitializeBsp()
{
	mutexInitialize(&taskingIdLock);

	auto numProcs = processorGetNumberOfProcessors();
	taskingLocal = (g_tasking_local*) heapAllocate(sizeof(g_tasking_local) * numProcs);
	taskGlobalMap = hashmapCreateNumeric<g_tid, g_task*>(128);

	if(numProcs > 1)
	{
		logInfo("%! disabling video log to boot on %i processors", "tasking", numProcs);
		/**
		 * TODO: The logger causes issues an multi-processor systems since
		 * it doesn't use a mutex but only disables interrupts. This is fine
		 * on single-core systems but causes issues here. Should be fixed
		 * by using the mutex there properly.
		 */
		loggerEnableVideo(false);
	}

	taskingInitializeLocal();
	taskingDirectoryInitialize();
}

void taskingInitializeAp()
{
	taskingInitializeLocal();
}

void taskingInitializeLocal()
{
	g_tasking_local* local = taskingGetLocal();
	local->time = 0;
	local->lockCount = 0;

	local->scheduling.current = 0;
	local->scheduling.list = 0;
	local->scheduling.round = 0;
	local->scheduling.idleTask = 0;

	mutexInitialize(&local->lock);

	g_process* idle = taskingCreateProcess();
	local->scheduling.idleTask = taskingCreateThread((g_virtual_address) taskingIdleThread, idle, G_SECURITY_LEVEL_KERNEL);
	local->scheduling.idleTask->type = G_THREAD_TYPE_VITAL;
	logInfo("%! core: %i idle task: %i", "tasking", processorGetCurrentId(), idle->main->id);

	g_process* cleanup = taskingCreateProcess();
	g_task* cleanupTask = taskingCreateThread((g_virtual_address) taskingCleanupThread, cleanup, G_SECURITY_LEVEL_KERNEL);
	cleanupTask->type = G_THREAD_TYPE_VITAL;
	taskingAssign(taskingGetLocal(), cleanupTask);
	logInfo("%! core: %i cleanup task: %i", "tasking", processorGetCurrentId(), cleanup->main->id);

	schedulerInitializeLocal();
}

void taskingApplySecurityLevel(volatile g_processor_state* state, g_security_level securityLevel)
{
	if(securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		state->cs = G_GDT_DESCRIPTOR_KERNEL_CODE | G_SEGMENT_SELECTOR_RING0;
		state->ss = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->ds = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->es = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->fs = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->gs = G_GDT_DESCRIPTOR_USERTHREADPTR;
	}
	else
	{
		state->cs = G_GDT_DESCRIPTOR_USER_CODE | G_SEGMENT_SELECTOR_RING3;
		state->ss = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->ds = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->es = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->fs = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->gs = G_GDT_DESCRIPTOR_USERTHREADPTR;
	}

	if(securityLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		state->eflags |= 0x3000; // IOPL 3
	}
}

void taskingResetTaskState(g_task* task)
{
	memorySetBytes((void*) task->state, 0, sizeof(g_processor_state));
	task->state->eflags = 0x200;
	task->state->esp = (g_virtual_address) task->state;
	taskingApplySecurityLevel(task->state, task->securityLevel);
}

void taskingAddToProcessTaskList(g_process* process, g_task* task)
{
	mutexAcquire(&process->lock);
	g_task_entry* entry = (g_task_entry*) heapAllocate(sizeof(g_task_entry));
	entry->task = task;
	entry->next = process->tasks;
	process->tasks = entry;
	if(process->main == 0)
	{
		process->main = task;
		process->id = task->id;
		filesystemProcessCreate((g_pid) task->id);
	}
	mutexRelease(&process->lock);
}

g_task* taskingCreateThread(g_virtual_address eip, g_process* process, g_security_level level)
{
	g_task* task = (g_task*) heapAllocateClear(sizeof(g_task));
	if(process->main == 0)
	{
		task->id = process->id;
	}
	else
	{
		task->id = taskingGetNextId();
	}
	task->process = process;
	task->securityLevel = level;
	task->status = G_THREAD_STATUS_RUNNING;
	task->type = G_THREAD_TYPE_DEFAULT;
	task->active = false;

	// Create task space
	g_physical_address returnDirectory = taskingTemporarySwitchToSpace(task->process->pageDirectory);

	taskingMemoryCreateStacks(task);
	taskingResetTaskState(task);
	task->state->eip = eip;

	if(task->securityLevel != G_SECURITY_LEVEL_KERNEL)
	{
		taskingPrepareThreadLocalStorage(task);
	}

	taskingTemporarySwitchBack(returnDirectory);

	// Put in process task list & global map
	taskingAddToProcessTaskList(process, task);
	hashmapPut(taskGlobalMap, task->id, task);
	return task;
}

g_task* taskingCreateThreadVm86(g_process* process, uint32_t intr, g_vm86_registers in, g_vm86_registers* out)
{
	g_task* task = (g_task*) heapAllocateClear(sizeof(g_task));
	task->id = taskingGetNextId();
	task->process = process;
	task->securityLevel = G_SECURITY_LEVEL_KERNEL;
	task->status = G_THREAD_STATUS_RUNNING;
	task->type = G_THREAD_TYPE_VM86;

	// Create task space
	g_physical_address returnDirectory = taskingTemporarySwitchToSpace(task->process->pageDirectory);
	task->interruptStack = taskingMemoryCreateStack(memoryVirtualRangePool, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS, G_TASKING_MEMORY_INTERRUPT_STACK_PAGES);

	g_processor_state_vm86* state = (g_processor_state_vm86*) (task->interruptStack.end - sizeof(g_processor_state_vm86));
	task->state = (g_processor_state*) state;

	memorySetBytes(state, 0, sizeof(g_processor_state_vm86));
	state->defaultFrame.eax = in.ax;
	state->defaultFrame.ebx = in.bx;
	state->defaultFrame.ecx = in.cx;
	state->defaultFrame.edx = in.dx;
	state->defaultFrame.ebp = 0;
	state->defaultFrame.esi = in.si;
	state->defaultFrame.edi = in.di;

	state->defaultFrame.eip = G_FP_OFF(ivt->entry[intr]);
	state->defaultFrame.cs = G_FP_SEG(ivt->entry[intr]);
	state->defaultFrame.eflags = 0x20202;
	g_virtual_address userStackVirt = (uint32_t) lowerHeapAllocate(2 * G_PAGE_SIZE);
	state->defaultFrame.ss = ((G_PAGE_ALIGN_DOWN(userStackVirt) + G_PAGE_SIZE) >> 4);

	state->gs = 0x00;
	state->fs = 0x00;
	state->es = in.es;
	state->ds = in.ds;

	task->vm86Data = (g_task_information_vm86*) heapAllocateClear(sizeof(g_task_information_vm86));
	task->vm86Data->userStack = userStackVirt;
	task->vm86Data->out = out;

	taskingTemporarySwitchBack(returnDirectory);

	// Put in process task list & global map
	taskingAddToProcessTaskList(process, task);
	hashmapPut(taskGlobalMap, task->id, task);
	return task;
}

void taskingAssign(g_tasking_local* local, g_task* task)
{
	mutexAcquire(&local->lock);

	bool alreadyInList = false;
	g_schedule_entry* existing = local->scheduling.list;
	while(existing)
	{
		if(existing->task == task)
		{
			alreadyInList = true;
			break;
		}
		existing = existing->next;
	}

	if(!alreadyInList)
	{
		g_schedule_entry* newEntry = (g_schedule_entry*) heapAllocate(sizeof(g_schedule_entry));
		newEntry->task = task;
		newEntry->next = local->scheduling.list;
		schedulerPrepareEntry(newEntry);
		local->scheduling.list = newEntry;
	}

	task->assignment = local;

	mutexRelease(&local->lock);
}

void taskingApplySwitch()
{
	g_task* task = taskingGetCurrentTask();
	if(!task)
	{
		kernelPanic("%! tried to restore without a current task", "tasking");
	}

	task->active = true;

	// Switch to process address space
	if(task->overridePageDirectory)
	{
		pagingSwitchToSpace(task->overridePageDirectory);
	}
	else
	{
		pagingSwitchToSpace(task->process->pageDirectory);
	}

	// For TLS: write user thread address to GDT & set GS of thread to user pointer segment
	gdtSetUserThreadObjectAddress(task->tlsCopy.userThreadObject);

#warning "TODO: Check if this is required:"
	task->state->gs = G_GDT_DESCRIPTOR_USERTHREADPTR;

	// Set TSS ESP0 for ring 3 tasks to return onto
	gdtSetTssEsp0(task->interruptStack.end);
}

void taskingSchedule()
{
	schedulerSchedule(taskingGetLocal());
	taskingApplySwitch();
}

g_process* taskingCreateProcess()
{
	g_process* process = (g_process*) heapAllocate(sizeof(g_process));
	process->id = taskingGetNextId();
	process->main = 0;
	process->tasks = 0;

	mutexInitialize(&process->lock);

	process->tlsMaster.size = 0;
	process->tlsMaster.location = 0;
	process->tlsMaster.userThreadOffset = 0;

	process->pageDirectory = taskingMemoryCreatePageDirectory();

	process->virtualRangePool = (g_address_range_pool*) heapAllocate(sizeof(g_address_range_pool));
	addressRangePoolInitialize(process->virtualRangePool);
	addressRangePoolAddRange(process->virtualRangePool, G_CONST_USER_VIRTUAL_RANGES_START, G_CONST_USER_VIRTUAL_RANGES_END);

	process->heap.brk = 0;
	process->heap.start = 0;
	process->heap.pages = 0;

	process->environment.arguments = 0;
	process->environment.executablePath = 0;
	process->environment.workingDirectory = 0;

	return process;
}

void taskingPrepareThreadLocalStorage(g_task* thread)
{
	// if tls master copy available, copy it to thread
	g_process* process = thread->process;
	if(process->tlsMaster.location == 0)
	{
		logDebug("%! while loading task %i: process %i does not have a tls", "tls", thread->id, process->id);
		return;
	}

	// allocate virtual range with required size
	uint32_t requiredSize = process->tlsMaster.size;
	uint32_t requiredPages = G_PAGE_ALIGN_UP(requiredSize) / G_PAGE_SIZE;
	g_virtual_address tlsCopyStart = addressRangePoolAllocate(process->virtualRangePool, requiredPages);
	g_virtual_address tlsCopyEnd = tlsCopyStart + requiredPages * G_PAGE_SIZE;

	// copy tls contents
	for(g_virtual_address page = tlsCopyStart; page < tlsCopyEnd; page += G_PAGE_SIZE)
	{
		g_physical_address phys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pagingMapPage(page, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		pageReferenceTrackerIncrement(phys);
	}

	// zero & copy TLS content
	memorySetBytes((void*) tlsCopyStart, 0, process->tlsMaster.size);
	memoryCopy((void*) tlsCopyStart, (void*) process->tlsMaster.location, process->tlsMaster.size);

	// fill user thread
	g_virtual_address userThreadObject = tlsCopyStart + process->tlsMaster.userThreadOffset;
	g_user_thread* userThread = (g_user_thread*) userThreadObject;
	userThread->self = userThread;

	// set threads TLS location
	thread->tlsCopy.userThreadObject = userThreadObject;
	thread->tlsCopy.start = tlsCopyStart;
	thread->tlsCopy.end = tlsCopyEnd;

	logDebug("%! created tls copy in process %i, thread %i at %h", "threadmgr", process->main->id, thread->id, thread->tlsCopy.start);
}

void taskingIdleThread()
{
	for(;;)
	{
		asm("hlt");
	}
}

void taskingCleanupThread()
{
	g_tasking_local* local = taskingGetLocal();
	g_task* task = taskingGetCurrentTask();
	for(;;)
	{
		// Find and remove dead tasks from local scheduling list
		mutexAcquire(&local->lock);

		g_schedule_entry* deadList = 0;
		g_schedule_entry* entry = local->scheduling.list;
		g_schedule_entry* previous = 0;
		while(entry)
		{
			g_schedule_entry* next = entry->next;
			if(entry->task->status == G_THREAD_STATUS_DEAD)
			{
				if(previous)
					previous->next = next;
				else
					local->scheduling.list = next;

				entry->next = deadList;
				deadList = entry;
			}
			else
			{
				previous = entry;
			}
			entry = next;
		}

		mutexRelease(&local->lock);

		// Remove each task
		while(deadList)
		{
			g_schedule_entry* next = deadList->next;
			taskingRemoveThread(deadList->task);
			heapFree(deadList);
			deadList = next;
		}

		// Sleep for some time
		waitSleep(task, 3000);
	}
}

void taskingRemoveThread(g_task* task)
{
	if(task->status != G_THREAD_STATUS_DEAD)
		kernelPanic("%! tried to remove a task %i that is not dead", "tasking", task->id);

	// Switch to task space
	g_physical_address returnDirectory = taskingTemporarySwitchToSpace(task->process->pageDirectory);

	// Clean up miscellaneous memory
	messageTaskRemoved(task->id);

	/* Remove interrupt stack */
	if(task->interruptStack.start)
	{
		for(g_virtual_address page = task->interruptStack.start; page < task->interruptStack.end; page += G_PAGE_SIZE)
		{
			g_physical_address pagePhys = pagingVirtualToPhysical(page);
			if(pagePhys > 0)
			{
				if(pageReferenceTrackerDecrement(pagePhys) == 0)
					bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, pagePhys);
				pagingUnmapPage(page);
			}
		}
		addressRangePoolFree(memoryVirtualRangePool, task->interruptStack.start);
	}
	for(g_virtual_address page = task->stack.start; page < task->stack.end; page += G_PAGE_SIZE)
	{
		g_physical_address pagePhys = pagingVirtualToPhysical(page);
		if(pagePhys > 0)
		{
			if(pageReferenceTrackerDecrement(pagePhys) == 0)
				bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, pagePhys);
			pagingUnmapPage(page);
		}
	}

	/* Remove user stacks */
	if(task->type == G_THREAD_TYPE_VM86)
	{
		lowerHeapFree((void*) task->vm86Data->userStack);
	}
	else if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		addressRangePoolFree(memoryVirtualRangePool, task->stack.start);
	}
	else
	{
		addressRangePoolFree(task->process->virtualRangePool, task->stack.start);
	}

	/* Free TLS copy if available */
	if(task->tlsCopy.start)
	{
		for(g_virtual_address page = task->tlsCopy.start; page < task->tlsCopy.end; page += G_PAGE_SIZE)
		{
			g_physical_address pagePhys = pagingVirtualToPhysical(page);
			if(pagePhys > 0)
			{
				if(pageReferenceTrackerDecrement(pagePhys) == 0)
					bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, pagePhys);
				pagingUnmapPage(page);
			}
		}
		addressRangePoolFree(task->process->virtualRangePool, task->tlsCopy.start);
	}

	taskingTemporarySwitchBack(returnDirectory);

	/* Remove self from process */
	mutexAcquire(&task->process->lock);

	g_task_entry* entry = task->process->tasks;
	g_task_entry* previous = 0;
	while(entry)
	{
		if(entry->task == task)
		{
			if(previous)
			{
				previous->next = entry->next;
			}
			else
			{
				task->process->tasks = entry->next;
			}
			heapFree(entry);
			break;
		}
		previous = entry;
		entry = entry->next;
	}

	mutexRelease(&task->process->lock);

	/* Kill process if necessary */
	if(task->process->tasks == 0)
	{
		taskingRemoveProcess(task->process);
	}
	else if(task->process->main == task)
	{
		taskingKillProcess(task->process->id);
	}

	/* Finalize freeing */
	hashmapRemove(taskGlobalMap, task->id);
	if(task->vm86Data)
		heapFree(task->vm86Data);
	heapFree(task);
}

void taskingKillProcess(g_pid pid)
{
	g_task* task = hashmapGet<g_pid, g_task*>(taskGlobalMap, pid, 0);
	if(!task)
	{
		logInfo("%! tried to kill non-existing process %i", "tasking", pid);
		return;
	}

	mutexAcquire(&task->process->lock);

	g_task_entry* entry = task->process->tasks;
	while(entry)
	{
		entry->task->status = G_THREAD_STATUS_DEAD;
		entry = entry->next;
	}

	mutexRelease(&task->process->lock);
}

void taskingRemoveProcess(g_process* process)
{
	mutexAcquire(&process->lock);

	filesystemProcessRemove(process->id);

	g_physical_address returnDirectory = taskingTemporarySwitchToSpace(process->pageDirectory);

	// Clear mappings and free physical space above 4 MiB
	g_page_directory directoryCurrent = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	for(uint32_t ti = 1; ti < 1024; ti++)
	{
		if((directoryCurrent[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE)
		{
			g_page_table table = ((g_page_table) G_CONST_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);
			for(uint32_t pi = 0; pi < 1024; pi++)
			{
				if(table[pi])
				{
					g_physical_address page = table[pi] & ~G_PAGE_ALIGN_MASK;

					int rem = pageReferenceTrackerDecrement(page);
					if(rem == 0)
					{
						bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, page);
					}
				}
			}
		}
	}

	taskingTemporarySwitchBack(returnDirectory);
	mutexRelease(&process->lock);

	heapFree(process->virtualRangePool);
	bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, process->pageDirectory);
	heapFree(process);
}

g_physical_address taskingTemporarySwitchToSpace(g_physical_address pageDirectory)
{
	g_physical_address back = pagingGetCurrentSpace();
	g_tasking_local* local = taskingGetLocal();
	if(local->scheduling.current)
	{
		if(local->scheduling.current->overridePageDirectory != 0)
			kernelPanic("%! %i tried temporary directory switching twice", "tasking", local->scheduling.current->id);

		local->scheduling.current->overridePageDirectory = pageDirectory;
	}
	pagingSwitchToSpace(pageDirectory);
	return back;
}

void taskingTemporarySwitchBack(g_physical_address back)
{
	g_tasking_local* local = taskingGetLocal();
	if(local->scheduling.current)
	{
		local->scheduling.current->overridePageDirectory = 0;
	}
	pagingSwitchToSpace(back);
}

g_spawn_status taskingSpawn(g_task* spawner, g_fd file, g_security_level securityLevel,
							g_process** outProcess, g_spawn_validation_details* outValidationDetails)
{
	return elfLoadExecutable(spawner, file, securityLevel, outProcess, outValidationDetails);
}
