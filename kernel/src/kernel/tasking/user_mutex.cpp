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
#include "kernel/tasking/clock.hpp"

static g_user_mutex nextMutex;
static g_mutex globalLock;
static g_hashmap<g_user_mutex, g_user_mutex_entry*>* mutexMap;

void userMutexInitialize()
{
	mutexInitializeGlobal(&globalLock, __func__);
	nextMutex = 0;
	mutexMap = hashmapCreateNumeric<g_user_mutex, g_user_mutex_entry*>(128);
}

g_user_mutex userMutexCreate(bool reentrant)
{
	auto entry = (g_user_mutex_entry*) heapAllocateClear(sizeof(g_user_mutex_entry));
	entry->reentrant = reentrant;
	entry->owner = G_TID_NONE;
	entry->creator = taskingGetCurrentTask()->process->id;
	waitQueueInitialize(&entry->waiters);
	mutexAcquire(&globalLock);
	g_user_mutex mutex = ++nextMutex;
	hashmapPut(mutexMap, mutex, entry);
	mutexRelease(&globalLock);
	return mutex;
}

static bool _userMutexTryAcquire(g_task* task, g_user_mutex_entry* entry)
{
	if(entry->value && !(entry->reentrant && entry->owner == task->id))
		return false;
	++entry->value;
	entry->owner = task->id;
	return true;
}

g_user_mutex_status userMutexTryAcquire(g_task* task, g_user_mutex mutex)
{
	mutexAcquire(&globalLock);
	auto entry = hashmapGet(mutexMap, mutex, (g_user_mutex_entry*) nullptr);
	bool acquired = entry && _userMutexTryAcquire(task, entry);
	mutexRelease(&globalLock);
	return acquired ? G_USER_MUTEX_STATUS_ACQUIRED : G_USER_MUTEX_STATUS_NOT_ACQUIRED;
}

g_user_mutex_status userMutexAcquire(g_task* task, g_user_mutex mutex, uint64_t timeout, bool trying)
{
	if(timeout)
		clockWaitForTime(task->id, clockGetLocal()->time + timeout);
	g_user_mutex_status result = G_USER_MUTEX_STATUS_NOT_ACQUIRED;
	while(true)
	{
		bool waiting = taskingWaitIf([&]()
		{
			mutexAcquire(&globalLock);
			auto entry = hashmapGet(mutexMap, mutex, (g_user_mutex_entry*) nullptr);
			bool wait = false;
			if(timeout && clockHasTimedOut(task->id))
				result = G_USER_MUTEX_STATUS_TIMEOUT;
			else if(entry && _userMutexTryAcquire(task, entry))
				result = G_USER_MUTEX_STATUS_ACQUIRED;
			else if(entry && !trying)
			{
				taskingPrepareWait(task, "user-mutex");
				waitQueueAdd(&entry->waiters, task->id);
				wait = true;
			}
			mutexRelease(&globalLock);
			return wait;
		});
		if(!waiting)
			break;
	}
	if(timeout)
		clockUnwaitForTime(task->id);
	userMutexUnwaitForAcquire(mutex, task->id);
	return result;
}

void userMutexRelease(g_user_mutex mutex)
{
	mutexAcquire(&globalLock);
	auto entry = hashmapGet(mutexMap, mutex, (g_user_mutex_entry*) nullptr);
	if(entry && entry->value > 0)
	{
		if(!entry->reentrant || --entry->value == 0)
		{
			entry->value = 0;
			entry->owner = G_TID_NONE;
			waitQueueWake(&entry->waiters);
		}
	}
	mutexRelease(&globalLock);
}

static void _userMutexDestroy(g_user_mutex mutex)
{
	auto entry = hashmapTake(mutexMap, mutex, (g_user_mutex_entry*) nullptr);
	if(entry)
	{
		waitQueueWake(&entry->waiters);
		heapFree(entry);
	}
}

void userMutexDestroy(g_user_mutex mutex)
{
	mutexAcquire(&globalLock);
	_userMutexDestroy(mutex);
	mutexRelease(&globalLock);
}

void userMutexTaskRemoved(g_tid task)
{
	mutexAcquire(&globalLock);
	auto iter = hashmapIteratorStart(mutexMap);
	while(hashmapIteratorHasNext(&iter))
	{
		auto entry = hashmapIteratorNext(&iter)->value;
		if(entry->owner == task)
		{
			entry->owner = G_TID_NONE;
			entry->value = 0;
			waitQueueWake(&entry->waiters);
		}
	}
	hashmapIteratorEnd(&iter);
	mutexRelease(&globalLock);
}

void userMutexProcessRemoved(g_pid process)
{
	mutexAcquire(&globalLock);
	while(true)
	{
		g_user_mutex owned = 0;
		auto iter = hashmapIteratorStart(mutexMap);
		while(hashmapIteratorHasNext(&iter))
		{
			auto item = hashmapIteratorNext(&iter);
			if(item->value->creator == process)
			{
				owned = item->key;
				break;
			}
		}
		hashmapIteratorEnd(&iter);
		if(!owned)
			break;
		_userMutexDestroy(owned);
	}
	mutexRelease(&globalLock);
}

void userMutexWaitForAcquire(g_user_mutex mutex, g_tid task)
{
	mutexAcquire(&globalLock);
	auto entry = hashmapGet(mutexMap, mutex, (g_user_mutex_entry*) nullptr);
	if(entry)
		waitQueueAdd(&entry->waiters, task);
	mutexRelease(&globalLock);
}

void userMutexUnwaitForAcquire(g_user_mutex mutex, g_tid task)
{
	mutexAcquire(&globalLock);
	auto entry = hashmapGet(mutexMap, mutex, (g_user_mutex_entry*) nullptr);
	if(entry)
		waitQueueRemove(&entry->waiters, task);
	mutexRelease(&globalLock);
}
