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

#include "kernel/tasking/atoms.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/utils/hashmap.hpp"
#include "kernel/tasking/clock.hpp"
#include "shared/logger/logger.hpp"

static g_atom nextAtom;
static g_mutex fullLock;
static g_hashmap<g_atom, g_atom_entry*>* atomMap;

void _atomicSetTaskWaiting(g_atom_entry* entry, g_task* task);
void _atomicWakeWaitingTasks(g_atom_entry* entry);

void atomicInitialize()
{
	mutexInitialize(&fullLock);
	nextAtom = 0;
	atomMap = hashmapCreateNumeric<g_atom, g_atom_entry*>(128);
}

g_atom atomicCreate(bool reentrant)
{
	g_atom_entry* entry = (g_atom_entry*) heapAllocate(sizeof(g_atom_entry));
	mutexInitialize(&entry->lock);
	entry->value = 0;
	entry->waiters = nullptr;
	entry->reentrant = reentrant;
	entry->owner = -1;

	mutexAcquire(&fullLock);
	g_atom atom = ++nextAtom;
	mutexRelease(&fullLock);
	hashmapPut(atomMap, atom, entry);

	return atom;
}

g_atom_lock_status atomicTryLock(g_task* task, g_atom atom)
{
	g_atom_entry* entry = hashmapGet<g_atom, g_atom_entry*>(atomMap, atom, nullptr);
	if(!entry)
	{
		logWarn("%! task %i tried to lock unknown atom %i", "atoms", task->id, atom);
		return false;
	}

	g_atom_lock_status status;
	mutexAcquire(&entry->lock);
	if(entry->value)
	{
		if(entry->reentrant && task->id == entry->owner)
		{
			entry->value++;
			status = G_ATOM_LOCK_STATUS_SET;
		}
		else
			status = G_ATOM_LOCK_STATUS_NOT_SET;
	}
	else
	{
		status = G_ATOM_LOCK_STATUS_SET;
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

g_atom_lock_status atomicLock(g_task* task, g_atom atom, uint64_t timeout, bool trying)
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

		g_atom_lock_status lockStatus = atomicTryLock(task, atom);
		if(wasSet = (lockStatus == G_ATOM_LOCK_STATUS_SET))
			break;

		if(trying)
			break;

		atomicWaitForLock(atom, task->id);
		task->status = G_THREAD_STATUS_WAITING;
		taskingYield();
	}

	if(useTimeout)
		clockUnwaitForTime(task->id);

	atomicUnwaitForLock(atom, task->id);

	return hasTimeout ? G_ATOM_LOCK_STATUS_TIMEOUT : (wasSet ? G_ATOM_LOCK_STATUS_SET : G_ATOM_LOCK_STATUS_NOT_SET);
}

void atomicUnlock(g_atom atom)
{
	g_atom_entry* entry = hashmapGet<g_atom, g_atom_entry*>(atomMap, atom, nullptr);
	if(!entry)
	{
		logWarn("%! task %i tried to unlock unknown atom %i", "atoms", taskingGetCurrentTask()->id, atom);
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
			_atomicWakeWaitingTasks(entry);
		}
	}
	else
	{
		entry->value = 0;
		_atomicWakeWaitingTasks(entry);
	}
	mutexRelease(&entry->lock);
}

void atomicDestroy(g_atom atom)
{
	atomicUnlock(atom);

	mutexAcquire(&atomMap->lock);
	g_atom_entry* entry = hashmapGet<g_atom, g_atom_entry*>(atomMap, atom, nullptr);
	if(entry)
	{
		hashmapRemove(atomMap, atom);
		heapFree(entry);
	}
	mutexRelease(&atomMap->lock);
}

void atomicWaitForLock(g_atom atom, g_tid task)
{
	g_atom_entry* entry = hashmapGet<g_atom, g_atom_entry*>(atomMap, atom, nullptr);
	if(!entry)
	{
		logWarn("%! tried to add waiter for task %i to unknown atom %i", "atoms", task, atom);
		return;
	}

	mutexAcquire(&entry->lock);

	auto waiter = (g_atom_waiter*) heapAllocate(sizeof(g_atom_waiter));
	waiter->task = task;
	waiter->next = entry->waiters;
	entry->waiters = waiter;

	mutexRelease(&entry->lock);
}

void atomicUnwaitForLock(g_atom atom, g_tid task)
{
	g_atom_entry* entry = hashmapGet<g_atom, g_atom_entry*>(atomMap, atom, nullptr);
	if(!entry)
	{
		logWarn("%! tried to remove waiter for task %i from unknown atom %i", "atoms", task, atom);
		return;
	}

	mutexAcquire(&entry->lock);

	g_atom_waiter* prev = nullptr;
	g_atom_waiter* waiter = entry->waiters;
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

void _atomicWakeWaitingTasks(g_atom_entry* entry)
{
	auto waiter = entry->waiters;
	while(waiter)
	{
		g_task* wakeTask = taskingGetById(waiter->task);
		if(wakeTask && wakeTask->status == G_THREAD_STATUS_WAITING)
		{
			wakeTask->status = G_THREAD_STATUS_RUNNING;
		}

		auto next = waiter->next;
		heapFree(waiter);
		waiter = next;
	}
	entry->waiters = nullptr;
}
