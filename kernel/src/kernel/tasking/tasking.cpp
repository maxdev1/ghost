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
#include "kernel/memory/gdt.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/system/interrupts/ivt.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/cleanup.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/elf/elf_loader.hpp"
#include "kernel/tasking/scheduler/scheduler.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "kernel/tasking/tasking_state.hpp"
#include "kernel/utils/hashmap.hpp"
#include "kernel/utils/wait_queue.hpp"
#include "shared/logger/logger.hpp"
#include "shared/panic.hpp"

static g_tasking_local* taskingLocal = 0;
static g_mutex taskingIdLock;
static g_tid taskingIdNext = 0;

g_hashmap<g_tid, g_task*>* taskGlobalMap;

void taskingInitializeTask(g_task* task, g_process* process, g_security_level level);

g_tasking_local* taskingGetLocal()
{
	return &taskingLocal[processorGetCurrentId()];
}

g_task* taskingGetCurrentTask()
{
	if(!systemIsReady())
		panic("%! can't access current task before initializing system", "tasking");

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

	if(taskingGetLocal()->lockCount)
		panic("%! can't yield while %i locks are held", "tasking", taskingGetLocal()->lockCount);

	g_task* task = taskingGetCurrentTask();
	task->statistics.timesYielded++;

	auto previousState = task->state;
	asm volatile("int $0x81" ::
					 : "cc", "memory");
	task->state = previousState;
}

void taskingIdleThread()
{
	for(;;)
	{
		asm volatile("hlt");
	}
}

void taskingExit()
{
	auto task = taskingGetCurrentTask();
	task->status = G_THREAD_STATUS_DEAD;
	waitQueueWake(&task->waitersJoin);
	taskingYield();
}

void taskingInitializeBsp()
{
	mutexInitialize(&taskingIdLock);

	auto numProcs = processorGetNumberOfProcessors();
	taskingLocal = (g_tasking_local*) heapAllocate(sizeof(g_tasking_local) * numProcs);
	taskGlobalMap = hashmapCreateNumeric<g_tid, g_task*>(128);

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
	local->lockCount = 0;
	local->lockSetIF = false;
	local->processor = processorGetCurrentId();

	local->scheduling.current = nullptr;
	local->scheduling.list = nullptr;
	local->scheduling.idleTask = nullptr;

	mutexInitialize(&local->lock);

	g_process* idle = taskingCreateProcess();
	local->scheduling.idleTask = taskingCreateTask((g_virtual_address) taskingIdleThread, idle, G_SECURITY_LEVEL_KERNEL);
	local->scheduling.idleTask->type = G_TASK_TYPE_VITAL;
	logInfo("%! core: %i idle task: %i", "tasking", processorGetCurrentId(), idle->main->id);

	// Before switching to the very first task, we must initialize this so that there is
	// an initial state for accessing kernel thread-local storage
	gdtSetTlsAddresses(nullptr, local->scheduling.idleTask->threadLocal.kernelThreadLocal);

	g_process* cleanup = taskingCreateProcess();
	g_task* cleanupTask = taskingCreateTask((g_virtual_address) taskingCleanupThread, cleanup, G_SECURITY_LEVEL_KERNEL);
	cleanupTask->type = G_TASK_TYPE_VITAL;
	taskingAssign(taskingGetLocal(), cleanupTask);
	logInfo("%! core: %i cleanup task: %i", "tasking", processorGetCurrentId(), cleanup->main->id);

	schedulerInitializeLocal();
}

void taskingProcessAddToTaskList(g_process* process, g_task* task)
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

void taskingProcessRemoveFromTaskList(g_task* task)
{
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
}

void taskingAssignBalanced(g_task* task)
{
	int lowestTaskCount = -1;
	g_tasking_local* assignTo;

	for(uint32_t proc = 0; proc < processorGetNumberOfProcessors(); proc++)
	{
		g_tasking_local* local = &taskingLocal[proc];
		mutexAcquire(&local->lock);

		int taskCount = 0;
		auto entry = local->scheduling.list;
		while(entry)
		{
			if(entry->task->status != G_THREAD_STATUS_DEAD)
				++taskCount;
			entry = entry->next;
		}

		if(lowestTaskCount == -1 || taskCount < lowestTaskCount)
		{
			lowestTaskCount = taskCount;
			assignTo = local;
		}

		mutexRelease(&local->lock);
	}

	taskingAssign(assignTo, task);
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

	// Add thread-local information on which processor this task runs now
	task->threadLocal.kernelThreadLocal->processor = local->processor;

	mutexRelease(&local->lock);
}

void taskingApplySwitch()
{
	g_task* task = taskingGetCurrentTask();
	if(!task)
	{
		panic("%! tried to restore without a current task", "tasking");
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

	// For TLS: write thread-local addresses to GDT
	gdtSetTlsAddresses(task->threadLocal.userThreadLocal, task->threadLocal.kernelThreadLocal);

	// Set TSS ESP0 for ring 3 tasks to return onto
	gdtSetTssEsp0(task->interruptStack.end);
}

void taskingSchedule()
{
	auto local = taskingGetLocal();
	schedulerSchedule(local);
	taskingApplySwitch();
}

g_process* taskingCreateProcess()
{
	// logInfo("heap used before process creation: %i", heapGetUsedAmount());

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
	addressRangePoolAddRange(process->virtualRangePool, G_USER_VIRTUAL_RANGES_START, G_USER_VIRTUAL_RANGES_END);

	process->heap.brk = 0;
	process->heap.start = 0;
	process->heap.pages = 0;

	process->environment.arguments = 0;
	process->environment.executablePath = 0;
	process->environment.workingDirectory = 0;

	return process;
}

void taskingDestroyProcess(g_process* process)
{
	if(process->object)
		elfObjectDestroy(process->object);

	filesystemProcessRemove(process->id);

	taskingMemoryDestroyPageDirectory(process->pageDirectory);

	addressRangePoolDestroy(process->virtualRangePool);
	heapFree(process->virtualRangePool);

	heapFree(process);

	// TODO there is still some heap wasting
	// logInfo("heap used after process destruction: %i", heapGetUsedAmount());
}

g_task* taskingCreateTask(g_virtual_address eip, g_process* process, g_security_level level)
{
	g_task* task = (g_task*) heapAllocateClear(sizeof(g_task));
	taskingInitializeTask(task, process, level);
	task->type = G_TASK_TYPE_DEFAULT;

	g_physical_address returnDirectory = taskingMemoryTemporarySwitchTo(task->process->pageDirectory);

	taskingMemoryCreateStacks(task);
	taskingStateReset(task, eip);
	taskingPrepareThreadLocalStorage(task);

	taskingMemoryTemporarySwitchBack(returnDirectory);

	taskingProcessAddToTaskList(process, task);
	hashmapPut(taskGlobalMap, task->id, task);

	return task;
}

g_task* taskingCreateTaskVm86(g_process* process, uint32_t intr, g_vm86_registers in, g_vm86_registers* out)
{
	g_task* task = (g_task*) heapAllocateClear(sizeof(g_task));
	taskingInitializeTask(task, process, G_SECURITY_LEVEL_KERNEL);
	task->type = G_TASK_TYPE_VM86;

	g_physical_address returnDirectory = taskingMemoryTemporarySwitchTo(task->process->pageDirectory);

	taskingMemoryCreateStacks(task);
	taskingStateResetVm86(task, in, intr);
	task->vm86Data = (g_task_information_vm86*) heapAllocateClear(sizeof(g_task_information_vm86));
	task->vm86Data->out = out;
	taskingPrepareThreadLocalStorage(task);

	taskingMemoryTemporarySwitchBack(returnDirectory);

	taskingProcessAddToTaskList(process, task);
	hashmapPut(taskGlobalMap, task->id, task);
	return task;
}

void taskingDestroyTask(g_task* task)
{
	if(task->status != G_THREAD_STATUS_DEAD)
		panic("%! tried to remove a task %i that is not dead", "tasking", task->id);

	// Wake up tasks that joined this task
	waitQueueWake(&task->waitersJoin);

	// Switch to task space
	g_physical_address returnDirectory = taskingMemoryTemporarySwitchTo(task->process->pageDirectory);

	messageTaskRemoved(task->id);
	// TODO cleanup other misc memory
	taskingMemoryDestroyStacks(task);
	taskingMemoryDestroyThreadLocalStorage(task);

	taskingMemoryTemporarySwitchBack(returnDirectory);

	// Remove from process
	taskingProcessRemoveFromTaskList(task);

	// Remove or kill process if necessary
	if(task->process->tasks == 0)
		taskingDestroyProcess(task->process);
	else if(task->process->main == task)
		taskingProcessKillAllTasks(task->process->id);

	// Finish cleanup
	hashmapRemove(taskGlobalMap, task->id);
	if(task->vm86Data)
		heapFree(task->vm86Data);
	heapFree(task);
}

void taskingInitializeTask(g_task* task, g_process* process, g_security_level level)
{
	if(process->main == 0)
		task->id = process->id;
	else
		task->id = taskingGetNextId();
	task->process = process;
	task->securityLevel = level;
	task->status = G_THREAD_STATUS_RUNNING;
	task->active = false;
	task->waitersJoin = nullptr;
}

void taskingProcessKillAllTasks(g_pid pid)
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

g_spawn_status taskingSpawn(g_task* spawner, g_fd file, g_security_level securityLevel,
							g_process** outProcess, g_spawn_validation_details* outValidationDetails)
{
	return elfLoadExecutable(spawner, file, securityLevel, outProcess, outValidationDetails);
}

void taskingWaitForExit(g_tid joinedTid, g_tid waiter)
{
	g_task* task = taskingGetById(joinedTid);
	if(!task)
		return;

	mutexAcquire(&task->process->lock);
	waitQueueAdd(&task->waitersJoin, waiter);
	mutexRelease(&task->process->lock);
}
