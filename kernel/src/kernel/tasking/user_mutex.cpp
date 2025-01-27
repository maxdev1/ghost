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

#include "kernel/tasking/user_mutex.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/utils/hashmap.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/tasking/clock.hpp"
#include "shared/logger/logger.hpp"

static g_user_mutex nextMutex;
static g_mutex globalLock;
static g_hashmap<g_user_mutex, g_user_mutex_entry*>* mutexMap;

void _userMutexSetTaskWaiting(g_user_mutex_entry* entry, g_task* task);
void _userMutexWakeWaitingTasks(g_user_mutex_entry* entry);

void userMutexInitialize()
{
	mutexInitializeNonInterruptible(&globalLock, __func__);
	nextMutex = 0;
	mutexMap = hashmapCreateNumeric<g_user_mutex, g_user_mutex_entry*>(128);
}

g_user_mutex userMutexCreate(bool reentrant)
{
	g_user_mutex_entry* entry = (g_user_mutex_entry*) heapAllocate(sizeof(g_user_mutex_entry));
	mutexInitialize(&entry->lock, __func__);
	entry->value = 0;
	entry->waiters = nullptr;
	entry->reentrant = reentrant;
	entry->owner = -1;

	mutexAcquire(&globalLock);
	g_user_mutex mutex = ++nextMutex;
	mutexRelease(&globalLock);
	hashmapPut(mutexMap, mutex, entry);

	return mutex;
}

g_user_mutex_status userMutexTryAcquire(g_task* task, g_user_mutex mutex)
{
	g_user_mutex_entry* entry = hashmapGet<g_user_mutex, g_user_mutex_entry*>(mutexMap, mutex, nullptr);
	if(!entry)
	{
		logWarn("%! task %i tried to lock unknown mutex %i", "mutex", task->id, mutex);
		return false;
	}

	g_user_mutex_status status;
	mutexAcquire(&entry->lock);
	if(entry->value)
	{
		if(entry->reentrant && task->id == entry->owner)
		{
			entry->value++;
			status = G_USER_MUTEX_STATUS_ACQUIRED;
		}
		else
			status = G_USER_MUTEX_STATUS_NOT_ACQUIRED;
	}
	else
	{
		status = G_USER_MUTEX_STATUS_ACQUIRED;
		entry->value = 1;
		if(entry->reentrant)
		{
			entry->owner = task->id;
		}
		else
			entry->owner = -1;
	}
	mutexRelease(&entry->lock);
	return status;
}

g_user_mutex_status userMutexAcquire(g_task* task, g_user_mutex mutex, uint64_t timeout, bool trying)
{
	bool wasSet = false;
	bool hasTimeout = false;

	bool useTimeout = (timeout > 0);
	if(useTimeout)
		clockWaitForTime(task->id, clockGetLocal()->time + timeout);

	while(true)
	{
		if(useTimeout && (hasTimeout = clockHasTimedOut(task->id)))
			break;

		g_user_mutex_status lockStatus = userMutexTryAcquire(task, mutex);
		if(wasSet = (lockStatus == G_USER_MUTEX_STATUS_ACQUIRED))
			break;

		if(trying)
			break;

		INTERRUPTS_PAUSE;
		userMutexWaitForAcquire(mutex, task->id);
		task->status = G_TASK_STATUS_WAITING;
		taskingYield();
		INTERRUPTS_RESUME;
	}

	if(useTimeout)
		clockUnwaitForTime(task->id);

	userMutexUnwaitForAcquire(mutex, task->id);

	return hasTimeout
		       ? G_USER_MUTEX_STATUS_TIMEOUT
		       : (wasSet ? G_USER_MUTEX_STATUS_ACQUIRED : G_USER_MUTEX_STATUS_NOT_ACQUIRED);
}

void userMutexRelease(g_user_mutex mutex)
{
	g_user_mutex_entry* entry = hashmapGet<g_user_mutex, g_user_mutex_entry*>(mutexMap, mutex, nullptr);
	if(!entry)
	{
		logWarn("%! task %i tried to unlock unknown mutex %i", "mutex", taskingGetCurrentTask()->id, mutex);
		return;
	}

	mutexAcquire(&entry->lock);
	if(entry->reentrant)
	{
		entry->value--;
		if(entry->value <= 0)
		{
			entry->value = 0;
			entry->owner = -1;
			_userMutexWakeWaitingTasks(entry);
		}
	}
	else
	{
		entry->value = 0;
		_userMutexWakeWaitingTasks(entry);
	}
	mutexRelease(&entry->lock);
}

void userMutexDestroy(g_user_mutex mutex)
{
	userMutexRelease(mutex);

	mutexAcquire(&mutexMap->lock);
	g_user_mutex_entry* entry = hashmapGet<g_user_mutex, g_user_mutex_entry*>(mutexMap, mutex, nullptr);
	if(entry)
	{
		hashmapRemove(mutexMap, mutex);
		heapFree(entry);
	}
	mutexRelease(&mutexMap->lock);
}

void userMutexWaitForAcquire(g_user_mutex mutex, g_tid task)
{
	g_user_mutex_entry* entry = hashmapGet<g_user_mutex, g_user_mutex_entry*>(mutexMap, mutex, nullptr);
	if(!entry)
	{
		logWarn("%! tried to add waiter for task %i to unknown mutex %i", "mutex", task, mutex);
		return;
	}

	mutexAcquire(&entry->lock);

	auto waiter = (g_user_mutex_waiter*) heapAllocate(sizeof(g_user_mutex_waiter));
	waiter->task = task;
	waiter->next = entry->waiters;
	entry->waiters = waiter;

	mutexRelease(&entry->lock);
}

void userMutexUnwaitForAcquire(g_user_mutex mutex, g_tid task)
{
	g_user_mutex_entry* entry = hashmapGet<g_user_mutex, g_user_mutex_entry*>(mutexMap, mutex, nullptr);
	if(!entry)
	{
		logWarn("%! tried to remove waiter for task %i from unknown mutex %i", "mutex", task, mutex);
		return;
	}

	mutexAcquire(&entry->lock);

	g_user_mutex_waiter* prev = nullptr;
	g_user_mutex_waiter* waiter = entry->waiters;
	while(waiter)
	{
		if(waiter->task == task)
		{
			auto next = waiter->next;
			if(prev)
				prev->next = next;
			else
				entry->waiters = next;

			heapFree(waiter);
			break;
		}
		prev = waiter;
		waiter = waiter->next;
	}

	mutexRelease(&entry->lock);
}

void _userMutexWakeWaitingTasks(g_user_mutex_entry* entry)
{
	auto waiter = entry->waiters;
	while(waiter)
	{
		g_task* wakeTask = taskingGetById(waiter->task);
		if(wakeTask && wakeTask->status == G_TASK_STATUS_WAITING)
		{
			wakeTask->status = G_TASK_STATUS_RUNNING;
		}

		auto next = waiter->next;
		heapFree(waiter);
		waiter = next;
	}
	entry->waiters = nullptr;
}
