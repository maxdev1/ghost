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

#include "kernel/tasking/wait.hpp"
#include "kernel/tasking/wait_resolver.hpp"

#include "kernel/memory/heap.hpp"
#include "shared/logger/logger.hpp"

bool waitTryWake(g_task* task)
{
	g_physical_address back = taskingTemporarySwitchToSpace(task->process->pageDirectory);

	bool wake = false;
	if(task->waitResolver && task->waitResolver(task))
	{
		task->waitResolver = 0;
		if(task->waitData)
		{
			heapFree((void*) task->waitData);
			task->waitData = 0;
		}

		task->status = G_THREAD_STATUS_RUNNING;
		wake = true;
	}

	taskingTemporarySwitchBack(back);
	return wake;
}

void waitSleep(g_task* task, uint64_t milliseconds)
{
	mutexAcquire(&task->process->lock);

	g_wait_resolver_sleep_data* waitData = (g_wait_resolver_sleep_data*) heapAllocate(sizeof(g_wait_resolver_sleep_data));
	waitData->wakeTime = taskingGetLocal()->time + milliseconds;
	task->waitData = waitData;
	task->waitResolver = waitResolverSleep;
	task->status = G_THREAD_STATUS_WAITING;

	mutexRelease(&task->process->lock);
}

void waitAtomicLock(g_task* task)
{
	mutexAcquire(&task->process->lock);

	g_wait_resolver_atomic_lock_data* waitData = (g_wait_resolver_atomic_lock_data*) heapAllocate(sizeof(g_wait_resolver_atomic_lock_data));
	waitData->startTime = taskingGetLocal()->time;
	task->waitData = waitData;
	task->waitResolver = waitResolverAtomicLock;
	task->status = G_THREAD_STATUS_WAITING;

	mutexRelease(&task->process->lock);
}

void waitForFile(g_task* task, g_fs_node* file, bool (*waitResolverFromDelegate)(g_task*))
{
	mutexAcquire(&task->process->lock);

	g_wait_resolver_for_file_data* waitData = (g_wait_resolver_for_file_data*) heapAllocate(sizeof(g_wait_resolver_for_file_data));
	waitData->waitResolverFromDelegate = waitResolverFromDelegate;
	waitData->nodeId = file->id;
	task->waitData = waitData;
	task->waitResolver = waitResolverFromDelegate;
	task->status = G_THREAD_STATUS_WAITING;

	mutexRelease(&task->process->lock);
}

void waitJoinTask(g_task* task, g_tid otherTask)
{
	mutexAcquire(&task->process->lock);

	g_wait_resolver_join_data* waitData = (g_wait_resolver_join_data*) heapAllocate(sizeof(g_wait_resolver_join_data));
	waitData->joinedTaskId = otherTask;
	task->waitData = waitData;
	task->waitResolver = waitResolverJoin;
	task->status = G_THREAD_STATUS_WAITING;

	mutexRelease(&task->process->lock);
}

void waitForMessageSend(g_task* task)
{
	mutexAcquire(&task->process->lock);

	task->waitData = 0;
	task->waitResolver = waitResolverSendMessage;
	task->status = G_THREAD_STATUS_WAITING;

	mutexRelease(&task->process->lock);
}

void waitForMessageReceive(g_task* task)
{
	mutexAcquire(&task->process->lock);

	task->waitData = 0;
	task->waitResolver = waitResolverReceiveMessage;
	task->status = G_THREAD_STATUS_WAITING;

	mutexRelease(&task->process->lock);
}

void waitForVm86(g_task* task, g_task* vm86Task, g_vm86_registers* registerStore)
{
	mutexAcquire(&task->process->lock);

	g_wait_vm86_data* waitData = (g_wait_vm86_data*) heapAllocate(sizeof(g_wait_vm86_data));
	waitData->registerStore = registerStore;
	waitData->vm86TaskId = vm86Task->id;
	task->waitData = waitData;
	task->waitResolver = waitResolverVm86;
	task->status = G_THREAD_STATUS_WAITING;

	mutexRelease(&task->process->lock);
}
