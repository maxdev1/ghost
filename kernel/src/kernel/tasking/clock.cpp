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
#include "kernel/tasking/tasking.hpp"
#include "kernel/utils/hashmap.hpp"
#include "shared/logger/logger.hpp"

/**
 * List of all tasks waiting for a specific time, ordered ascending by the wake-up time.
 */
static g_clock_waiter* waiters;
static g_mutex lock;

void clockInitialize()
{
	mutexInitialize(&lock);
	waiters = nullptr;
}

void clockWaitForTime(g_tid task, uint64_t wakeTime)
{
	mutexAcquire(&lock);

	g_clock_waiter* waiter = (g_clock_waiter*) heapAllocate(sizeof(g_clock_waiter));
	waiter->task = task;
	waiter->wakeTime = wakeTime;
	waiter->next = nullptr;

	if(waiters)
	{
		g_clock_waiter* prev = nullptr;
		g_clock_waiter* entry = waiters;
		while(entry)
		{
			// Insert in middle
			if(entry->wakeTime > wakeTime)
			{
				waiter->next = entry;
				if(prev)
					prev->next = waiter;
				else
					waiters = waiter;
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
		waiters = waiter;
	}

	mutexRelease(&lock);
}

void clockUpdate()
{
	auto time = taskingGetLocal()->time;

	mutexAcquire(&lock);

	while(waiters && time >= waiters->wakeTime)
	{
		g_task* task = taskingGetById(waiters->task);
		if(task)
		{
			task->status = G_THREAD_STATUS_RUNNING;
		}

		auto next = waiters->next;
		heapFree(waiters);
		waiters = next;
	}

	mutexRelease(&lock);
}

void clockUnwaitForTime(g_tid task)
{
	mutexAcquire(&lock);

	g_clock_waiter* prev = nullptr;
	g_clock_waiter* entry = waiters;
	while(entry)
	{
		if(entry->task == task)
		{
			auto next = entry->next;
			if(prev)
				prev->next = next;
			else
				waiters = next;

			heapFree(entry);
			entry = next;
		}
		else
		{
			prev = entry;
			entry = entry->next;
		}
	}

	mutexRelease(&lock);
}

bool clockHasTimedOut(g_tid task)
{
	auto time = taskingGetLocal()->time;
	bool timeout = true;

	mutexAcquire(&lock);

	g_clock_waiter* entry = waiters;
	while(entry)
	{
		if(entry->task == task && time < entry->wakeTime)
		{
			timeout = false;
			break;
		}
		entry = entry->next;
	}

	mutexRelease(&lock);

	return timeout;
}
