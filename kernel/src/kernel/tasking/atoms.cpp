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
void _atomicWakeNextWaiter(g_atom_entry* entry);

void atomicInitialize()
{
	mutexInitialize(&fullLock);
	nextAtom = 0;
	atomMap = hashmapCreateNumeric<g_atom, g_atom_entry*>(128);
}

g_atom atomicCreate()
{
	mutexAcquire(&fullLock);

	g_atom_entry* entry = (g_atom_entry*) heapAllocate(sizeof(g_atom_entry));
	mutexInitialize(&entry->lock);
	entry->value = 0;
	entry->waiters = nullptr;

	g_atom atom = ++nextAtom;
	hashmapPut(atomMap, atom, entry);

	mutexRelease(&fullLock);
	return atom;
}

bool atomicLock(g_task* task, g_atom atom, bool isTry, bool setOnFinish, uint64_t timeoutTime)
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
		if(!isTry)
			_atomicSetTaskWaiting(entry, task);
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
		logWarn("%! task %i tried to unlock unknown atom %i", "atoms", task->id, atom);
		return;
	}

	mutexAcquire(&entry->lock);
	entry->value = 0;
	_atomicWakeNextWaiter(entry);
	mutexRelease(&entry->lock);
}

void _atomicWakeNextWaiter(g_atom_entry* entry)
{
	auto waiter = entry->waiters;
	if(!waiter)
		return;

	g_task* wakeTask = taskingGetById(waiter->task);
	if(wakeTask && wakeTask->status == G_THREAD_STATUS_WAITING)
	{
		wakeTask->status = G_THREAD_STATUS_RUNNING;
	}

	entry->waiters = waiter->next;
	heapFree(waiter);
}

void _atomicSetTaskWaiting(g_atom_entry* entry, g_task* task)
{
	task->status = G_THREAD_STATUS_WAITING;

	g_atom_waiter* waiter = (g_atom_waiter*) heapAllocate(sizeof(g_atom_waiter));
	waiter->task = task->id;

	if(entry->waiters)
	{
		auto last = entry->waiters;
		while(last)
		{
			if(!last->next)
			{
				last->next = waiter;
				break;
			}
			last = last->next;
		}
	}
	else
	{
		waiter->next = nullptr;
		entry->waiters = waiter;
	}
}