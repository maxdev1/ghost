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

#include "kernel/tasking/clock.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/system/configuration.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/panic.hpp"

static g_clock_local* locals = 0;

void clockInitialize()
{
	uint32_t numProcs = processorGetNumberOfProcessors();
	locals = (g_clock_local*) heapAllocate(sizeof(g_clock_local) * numProcs);

	for(uint32_t i = 0; i < numProcs; i++)
	{
		mutexInitialize(&locals[i].lock);
		locals[i].waiters = nullptr;
	}
}

g_clock_local* clockGetLocal()
{
	if(!locals)
		panic("%! attempted to use clock before initializing it", "clock");

	return &locals[processorGetCurrentId()];
}

void clockWaitForTime(g_tid task, uint64_t wakeTime)
{
	auto local = clockGetLocal();
	mutexAcquire(&local->lock);

	g_clock_waiter* waiter = (g_clock_waiter*) heapAllocate(sizeof(g_clock_waiter));
	waiter->task = task;
	waiter->wakeTime = wakeTime;
	waiter->next = nullptr;

	if(local->waiters)
	{
		g_clock_waiter* prev = nullptr;
		g_clock_waiter* entry = local->waiters;
		while(entry)
		{
			// Insert in middle
			if(entry->wakeTime > wakeTime)
			{
				waiter->next = entry;
				if(prev)
					prev->next = waiter;
				else
					local->waiters = waiter;
				break;
			}
			prev = entry;
			entry = entry->next;

			// Insert at tail
			if(!entry)
				prev->next = waiter;
		}
	}
	else
	{
		// Insert at head
		local->waiters = waiter;
	}

	mutexRelease(&local->lock);
}

void clockUpdate()
{
	auto local = clockGetLocal();
	mutexAcquire(&local->lock);

	// Update local time
	local->time += (1000 / G_TIMER_FREQUENCY);

	// Wake waiters
	while(local->waiters && local->time >= local->waiters->wakeTime)
	{
		g_task* task = taskingGetById(local->waiters->task);
		if(task)
		{
			task->status = G_THREAD_STATUS_RUNNING;
		}

		auto next = local->waiters->next;
		heapFree(local->waiters);
		local->waiters = next;
	}

	mutexRelease(&local->lock);
}

void clockUnwaitForTime(g_tid task)
{
	auto local = clockGetLocal();
	mutexAcquire(&local->lock);

	g_clock_waiter* prev = nullptr;
	g_clock_waiter* entry = local->waiters;
	while(entry)
	{
		if(entry->task == task)
		{
			auto next = entry->next;
			if(prev)
				prev->next = next;
			else
				local->waiters = next;

			heapFree(entry);
			entry = next;
		}
		else
		{
			prev = entry;
			entry = entry->next;
		}
	}

	mutexRelease(&local->lock);
}

bool clockHasTimedOut(g_tid task)
{
	auto local = clockGetLocal();
	mutexAcquire(&local->lock);

	bool timeout = true;

	g_clock_waiter* entry = local->waiters;
	while(entry)
	{
		if(entry->task == task && local->time < entry->wakeTime)
		{
			timeout = false;
			break;
		}
		entry = entry->next;
	}

	mutexRelease(&local->lock);
	return timeout;
}
