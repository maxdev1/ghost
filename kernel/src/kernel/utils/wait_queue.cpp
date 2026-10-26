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

#include "kernel/utils/wait_queue.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/system/interrupts/interrupts.hpp"

static g_spinlock waitRegistryLock = 0;
static g_wait_queue_entry* waitRegistry = nullptr;

// One short lock protects both queue links and the reverse index. No heap or
// scheduler operations run while it is held, including on interrupt paths.
struct g_wait_registry_guard
{
	g_interrupt_guard interrupts;
	g_wait_registry_guard() { G_SPINLOCK_ACQUIRE(waitRegistryLock); }
	~g_wait_registry_guard() { G_SPINLOCK_RELEASE(waitRegistryLock); }
};

static void _waitQueueUnregister(g_wait_queue_entry* entry)
{
	if(entry->registryPrevious)
		entry->registryPrevious->registryNext = entry->registryNext;
	else
		waitRegistry = entry->registryNext;
	if(entry->registryNext)
		entry->registryNext->registryPrevious = entry->registryPrevious;
}

void waitQueueInitialize(g_wait_queue* queue)
{
	queue->head = nullptr;
}

void waitQueueAdd(g_wait_queue* queue, g_tid task)
{
	auto entry = (g_wait_queue_entry*) heapAllocate(sizeof(g_wait_queue_entry));
	bool duplicate = false;
	{
		g_wait_registry_guard guard;
		for(auto existing = queue->head; existing; existing = existing->next)
			if(existing->task == task)
			{
				// An unrelated wake-up can make a waiter retry registration.
				duplicate = true;
				break;
			}
		if(!duplicate)
		{
			entry->task = task;
			entry->queue = queue;
			entry->next = queue->head;
			queue->head = entry;
			entry->registryPrevious = nullptr;
			entry->registryNext = waitRegistry;
			if(waitRegistry)
				waitRegistry->registryPrevious = entry;
			waitRegistry = entry;
			return;
		}
	}
	heapFree(entry);
}

void waitQueueRemove(g_wait_queue* queue, g_tid task)
{
	g_wait_queue_entry* removed = nullptr;
	{
		g_wait_registry_guard guard;
		auto link = &queue->head;
		while(*link)
		{
			auto entry = *link;
			if(entry->task == task)
			{
				*link = entry->next;
				_waitQueueUnregister(entry);
				removed = entry;
				break;
			}
			link = &entry->next;
		}
	}
	if(removed)
		heapFree(removed);
}

void waitQueueRemoveTask(g_tid task)
{
	g_wait_queue_entry* removed = nullptr;
	{
		g_wait_registry_guard guard;
		auto entry = waitRegistry;
		while(entry)
		{
			auto next = entry->registryNext;
			if(entry->task == task)
			{
				auto link = &entry->queue->head;
				while(*link != entry)
					link = &(*link)->next;
				*link = entry->next;
				_waitQueueUnregister(entry);
				entry->next = removed;
				removed = entry;
			}
			entry = next;
		}
	}
	while(removed)
	{
		auto next = removed->next;
		heapFree(removed);
		removed = next;
	}
}

bool waitQueueIsEmpty(g_wait_queue* queue)
{
	g_wait_registry_guard guard;
	return queue->head == nullptr;
}

void waitQueueWake(g_wait_queue* queue)
{
	g_wait_queue_entry* waiter;
	{
		g_wait_registry_guard guard;
		waiter = queue->head;
		queue->head = nullptr;
		for(auto entry = waiter; entry; entry = entry->next)
			_waitQueueUnregister(entry);
	}
	while(waiter)
	{
		auto task = taskingAcquireTask(waiter->task);
		taskingWake(task);
		taskingReleaseTask(task);
		auto next = waiter->next;
		heapFree(waiter);
		waiter = next;
	}
}
