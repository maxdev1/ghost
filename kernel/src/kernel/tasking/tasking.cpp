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
#include "shared/memory/constants.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/ipc/message_queues.hpp"
#include "kernel/memory/gdt.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/cleanup.hpp"
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

void _taskingInitializeTask(g_task* task, g_process* process, g_security_level level);

g_tasking_local* taskingGetLocal() { return &taskingLocal[processorGetCurrentId()]; }

g_task* taskingGetCurrentTask()
{
	if(!systemIsReady())
		panic("%! can't access current task before initializing system", "tasking");

	return taskingGetLocal()->scheduling.current;
}

g_tid taskingGetNextId()
{
	mutexAcquire(&taskingIdLock);
	g_tid next = taskingIdNext++;
	mutexRelease(&taskingIdLock);
	return next;
}

g_task* taskingGetById(g_tid id) { return hashmapGet(taskGlobalMap, id, (g_task*) 0); }

/**
 * When yielding, store the state pointer (on top of the interrupt stack)
 * and when returned, restore this state.
 */
void taskingYield()
{
	if(!systemIsReady())
		return;

	if(taskingGetLocal()->locking.globalLockCount > 0)
		panic("%! attempted yield while holding global lock", "bug");

	g_task* task = taskingGetCurrentTask();
	task->statistics.timesYielded++;

	auto previousState = task->state;
	asm volatile("int $0x81" ::: "cc", "memory");
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
	mutexAcquire(&task->lock);
	task->status = G_TASK_STATUS_DEAD;
	waitQueueWake(&task->waitersJoin);
	mutexRelease(&task->lock);
	taskingYield();
}

void taskingInitializeBsp()
{
	mutexInitializeGlobal(&taskingIdLock, __func__);

	auto numProcs = processorGetNumberOfProcessors();
	taskingLocal = (g_tasking_local*) heapAllocate(sizeof(g_tasking_local) * numProcs);
	taskGlobalMap = hashmapCreateNumeric<g_tid, g_task*>(128);

	taskingInitializeLocal();
	taskingDirectoryInitialize();
}

void taskingInitializeAp() { taskingInitializeLocal(); }

void taskingInitializeLocal()
{
	g_tasking_local* local = taskingGetLocal();
	local->locking.globalLockCount = 0;
	local->locking.globalLockSetIFAfterRelease = false;
	local->processor = processorGetCurrentId();

	local->scheduling.current = nullptr;
	local->scheduling.list = nullptr;
	local->scheduling.idleTask = nullptr;

	mutexInitializeGlobal(&local->lock, __func__);

	schedulerInitializeLocal();
	taskingCreateEssentialTasks();
}

void taskingCreateEssentialTasks()
{
	g_tasking_local* local = taskingGetLocal();

	g_process* idle = taskingCreateProcess(G_SECURITY_LEVEL_KERNEL);
	local->scheduling.idleTask =
			taskingCreateTask((g_virtual_address) taskingIdleThread, idle, G_SECURITY_LEVEL_KERNEL);
	local->scheduling.idleTask->type = G_TASK_TYPE_VITAL;
	logInfo("%! core: %i idle task: %i", "tasking", processorGetCurrentId(), idle->main->id);

	// Before switching to the very first task, we must initialize this so that there is
	// an initial state for accessing kernel thread-local storage
	gdtSetTlsAddresses(nullptr, local->scheduling.idleTask->threadLocal.kernelThreadLocal);

	g_process* cleanup = taskingCreateProcess(G_SECURITY_LEVEL_KERNEL);
	g_task* cleanupTask = taskingCreateTask((g_virtual_address) taskingCleanupThread, cleanup, G_SECURITY_LEVEL_KERNEL);
	cleanupTask->type = G_TASK_TYPE_VITAL;
	taskingAssign(taskingGetLocal(), cleanupTask);
	logInfo("%! core: %i cleanup task: %i", "tasking", processorGetCurrentId(), cleanup->main->id);
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

	for(uint32_t core = 0; core < processorGetNumberOfProcessors(); core++)
	{
		g_tasking_local* local = &taskingLocal[core];
		mutexAcquire(&local->lock);

		int taskCount = 0;
		auto entry = local->scheduling.list;
		while(entry)
		{
			if(entry->task->status != G_TASK_STATUS_DEAD)
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

void taskingAssignOnCore(uint8_t core, g_task* task)
{
	if(core < processorGetNumberOfProcessors())
	{
		taskingAssign(&taskingLocal[core], task);
	}
	else
	{
		logInfo("%! failed to assign task to core %i since it exceeds number of processors", "tasking", core);
	}
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

void taskingSaveState(g_task* task, g_processor_state* state)
{
	// Save latest pointer to interrupt stack top
	task->state = state;

	// Save FPU state
	if(task->fpu.state)
	{
		processorSaveFpuState(task->fpu.state);
		task->fpu.stored = true;
	}
}


void taskingRestoreState(g_task* task)
{
	if(!task)
		panic("%! tried to restore without a current task", "tasking");

	// Switch to process address space
	if(task->overridePageDirectory)
	{
		pagingSwitchToSpace(task->overridePageDirectory);
	}
	else
	{
		pagingSwitchToSpace(task->process->pageSpace);
	}

	// For TLS: write thread-local addresses
	gdtSetTlsAddresses(task->threadLocal.userThreadLocal, task->threadLocal.kernelThreadLocal);

	// Set TSS RSP0 for ring 3 tasks to return onto
	gdtSetTssRsp0(task->interruptStack.end);

	// Restore FPU state
	if(task->fpu.stored)
		processorRestoreFpuState(task->fpu.state);
}

void taskingSchedule(bool resetPreference)
{
	if(resetPreference && processorIsBsp())
		schedulerPrefer(G_TID_NONE);
	schedulerSchedule(taskingGetLocal());
}

void taskingSetCurrent(g_task* task)
{
	schedulerSetCurrent(taskingGetLocal(), task);
}

g_process* taskingCreateProcess(g_security_level securityLevel)
{
	auto process = (g_process*) heapAllocateClear(sizeof(g_process));
	process->id = taskingGetNextId();

	mutexInitializeGlobal(&process->lock, __func__);

	process->pageSpace = taskingMemoryCreatePageSpace();
	if(!process->pageSpace)
	{
		logInfo("%! failed to create new address space to create process", "tasking");
		return nullptr;
	}

	process->virtualRangePool = (g_address_range_pool*) heapAllocate(sizeof(g_address_range_pool));
	addressRangePoolInitialize(process->virtualRangePool);
	addressRangePoolAddRange(process->virtualRangePool, G_USER_VIRTUAL_RANGES_START, G_USER_VIRTUAL_RANGES_END);

	logDebug("%! new process %i, address space %x", "tasking", process->id, process->pageSpace);
	return process;
}

void taskingDestroyProcess(g_process* process)
{
	if(process->object)
		elfObjectDestroy(process->object);

	filesystemProcessRemove(process->id);

	taskingMemoryDestroyPageSpace(process->pageSpace);

	addressRangePoolDestroy(process->virtualRangePool);
	heapFree(process->virtualRangePool);

	heapFree(process);

	// TODO there is still some heap wasting
	// logInfo("heap used after process destruction: %i", heapGetUsedAmount());
}

g_task* taskingCreateTask(g_virtual_address eip, g_process* process, g_security_level level)
{
	auto task = (g_task*) heapAllocateClear(sizeof(g_task));
	if(!task)
		return nullptr;

	_taskingInitializeTask(task, process, level);
	task->type = G_TASK_TYPE_DEFAULT;

	g_physical_address returnSpace = taskingMemoryTemporarySwitchTo(task->process->pageSpace);

	taskingMemoryInitialize(task);
	taskingStateReset(task, eip, level);

	logDebug("%! created task %i, intr stack: %x-%x, stack: %x-%x", "tasking", task->id, task->interruptStack.start,
			task->interruptStack.end, task->stack.start, task->stack.end);
	logDebug("%# state: RIP: %x, RSP: %x, CS: %h, SS: %h, RFLAGS: %h", task->state->rip, task->state->rsp, task->state->cs, task->state->ss, task->state->rflags);

	taskingMemoryTemporarySwitchBack(returnSpace);

	taskingProcessAddToTaskList(process, task);
	hashmapPut(taskGlobalMap, task->id, task);

	return task;
}

g_task* taskingCreateTaskVm86(g_process* process, uint32_t intr, g_vm86_registers in, g_vm86_registers* out)
{
	panic("no vm86");
}

void taskingDestroyTask(g_task* task)
{
	mutexAcquire(&task->lock);

	if(task->status != G_TASK_STATUS_DEAD)
		panic("%! tried to remove a task %i that is not dead", "tasking", task->id);

	// Wake up tasks that joined this task
	waitQueueWake(&task->waitersJoin);

	// Switch to task space
	g_physical_address returnDirectory = taskingMemoryTemporarySwitchTo(task->process->pageSpace);

	messageQueueTaskRemoved(task->id);
	taskingMemoryDestroy(task);

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

	mutexRelease(&task->lock);
	heapFree(task);
}

void _taskingInitializeTask(g_task* task, g_process* process, g_security_level level)
{
	if(process->main == nullptr)
		task->id = process->id;
	else
		task->id = taskingGetNextId();
	task->process = process;
	task->securityLevel = level;
	task->status = G_TASK_STATUS_RUNNING;
	waitQueueInitialize(&task->waitersJoin);
	mutexInitializeGlobal(&task->lock, __func__);
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
		entry->task->status = G_TASK_STATUS_DEAD;
		entry = entry->next;
	}

	mutexRelease(&task->process->lock);
}

g_spawn_result taskingSpawn(g_fd fd, g_security_level securityLevel)
{
	g_task* parent = taskingGetCurrentTask();

	// Create target process & task
	g_spawn_result res{};
	res.process = taskingCreateProcess(securityLevel);
	g_task* child = taskingCreateTask(0, res.process, securityLevel);
	if(!child)
	{
		logInfo("%! failed to create main thread to spawn binary", "elf");
		res.status = G_SPAWN_STATUS_TASKING_ERROR;
		return res;
	}

	// Clone FD to target process
	g_fd targetFd;
	auto cloneStat = filesystemProcessCloneDescriptor(parent->process->id, fd, res.process->id, G_FD_NONE, &targetFd);
	if(cloneStat != G_FS_CLONEFD_SUCCESSFUL)
	{
		logInfo("%! failed to clone binary FD to target process", "elf");
		res.status = G_SPAWN_STATUS_IO_ERROR;
		return res;
	}

	// Provide spawn arguments
	res.process->spawnArgs = (g_process_spawn_arguments*) heapAllocateClear(sizeof(g_process_spawn_arguments));
	res.process->spawnArgs->parent = parent->id;
	res.process->spawnArgs->fd = targetFd;
	res.process->spawnArgs->securityLevel = securityLevel;

	// Set kernel-level entry
	taskingStateReset(child, (g_address) &taskingSpawnEntry, G_SECURITY_LEVEL_KERNEL);

	// Start thread & wait for spawn to finish
	taskingWait(parent, __func__, [child]()
	{
		child->spawnFinished = false;
		taskingAssignBalanced(child);
	});

	// Take result
	res.status = res.process->spawnArgs->status;
	res.validation = res.process->spawnArgs->validation;

	// Clean up
	heapFree(res.process->spawnArgs);
	res.process->spawnArgs = nullptr;

	return res;
}

void taskingSpawnEntry()
{
	auto task = taskingGetCurrentTask();
	auto process = task->process;
	auto args = process->spawnArgs;

	// Load binary
	auto loadRes = elfLoadExecutable(args->fd, args->securityLevel);
	args->status = loadRes.status;
	args->validation = loadRes.validationDetails;

	if(loadRes.status != G_SPAWN_STATUS_SUCCESSFUL)
	{
		logInfo("%! failed to load binary to current address space", "elf");
		auto parent = taskingGetById(process->spawnArgs->parent);
		taskingWake(parent);
		taskingExit();
	}

	// Finalize initialization and do privilege downgrade
	interruptsDisable();
	taskingMemoryInitializeTls(task);
	args->entry = loadRes.entry;
	asm volatile("int $0x82" ::: "cc", "memory");
}

void taskingFinalizeSpawn(g_task* task)
{
	INTERRUPTS_PAUSE;
	auto process = task->process;
	task->securityLevel = process->spawnArgs->securityLevel;
	taskingStateReset(task, process->spawnArgs->entry, task->securityLevel);

	taskingWait(task, __func__, [task, process]()
	{
		task->spawnFinished = true;
		auto parent = taskingGetById(process->spawnArgs->parent);
		taskingWake(parent);
	});
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

void taskingWake(g_task* task)
{
	if(task)
	{
		mutexAcquire(&task->lock);
		if(task->status == G_TASK_STATUS_WAITING)
			task->status = G_TASK_STATUS_RUNNING;
		mutexRelease(&task->lock);
	}
}

void taskingWait(g_task* task, const char* debugName, const std::function<void ()>& beforeYield)
{
	INTERRUPTS_PAUSE;
	mutexAcquire(&task->lock);
	task->status = G_TASK_STATUS_WAITING;
	task->waitsFor = debugName;
	mutexRelease(&task->lock);
	if(beforeYield)
		beforeYield();
	taskingYield();
	INTERRUPTS_RESUME;
}
