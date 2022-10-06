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

g_atom atomicCreate()
{

	g_atom_entry* entry = (g_atom_entry*) heapAllocate(sizeof(g_atom_entry));
	mutexInitialize(&entry->lock);
	entry->value = 0;
	entry->waiters = nullptr;

	mutexAcquire(&fullLock);
	g_atom atom = ++nextAtom;
	mutexRelease(&fullLock);
	hashmapPut(atomMap, atom, entry);

	return atom;
}

bool atomicLock(g_task* task, g_atom atom, bool isTry, bool setOnFinish)
{
	g_atom_entry* entry = hashmapGet<g_atom, g_atom_entry*>(atomMap, atom, nullptr);
	if(!entry)
	{
		logWarn("%! task %i tried to lock unknown atom %i", "atoms", task->id, atom);
		return false;
	}

	bool ret;
	mutexAcquire(&entry->lock);
	if(entry->value)
	{
		ret = false;
	}
	else
	{
		ret = true;
		if(setOnFinish)
			entry->value = 1;
	}
	mutexRelease(&entry->lock);
	return ret;
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
	entry->value = 0;
	_atomicWakeWaitingTasks(entry);
	mutexRelease(&entry->lock);
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

	g_atom_waiter* waiter = (g_atom_waiter*) heapAllocate(sizeof(g_atom_waiter));
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
