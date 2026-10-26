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
#include "kernel/system/timing/hpet.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/panic.hpp"
#include "kernel/logger/logger.hpp"

static g_clock_local* locals = nullptr;

static g_clock_local* _clockGetLocalForProcessor(uint32_t processor)
{
	if(!locals)
		panic("%! attempted to use clock before initializing it", "clock");

	return &locals[processor];
}

static g_clock_local* _clockGetLocalForTask(g_task* task)
{
	if(task && task->assignment)
		return _clockGetLocalForProcessor(task->assignment->processor);

	return clockGetLocal();
}

static void _clockRemoveWaiter(g_clock_local* local, g_task* task)
{
	if(!task || !task->timeoutWait.active)
		return;

	g_task* previous = nullptr;
	g_task* entry = local->waiters;
	while(entry)
	{
		if(entry == task)
		{
			if(previous)
				previous->timeoutWait.next = entry->timeoutWait.next;
			else
				local->waiters = entry->timeoutWait.next;

			entry->timeoutWait.next = nullptr;
			entry->timeoutWait.wakeTime = 0;
			entry->timeoutWait.active = false;
			return;
		}

		previous = entry;
		entry = entry->timeoutWait.next;
	}

	task->timeoutWait.next = nullptr;
	task->timeoutWait.wakeTime = 0;
	task->timeoutWait.active = false;
}

void clockInitialize()
{
	uint32_t numProcs = processorGetNumberOfProcessors();
	locals = (g_clock_local*) heapAllocate(sizeof(g_clock_local) * numProcs);
	const uint64_t initialNanoTime = hpetGetNanos();

	for(uint32_t i = 0; i < numProcs; i++)
	{
		mutexInitializeGlobal(&locals[i].lock, __func__);
		locals[i].waiters = nullptr;
		locals[i].time = 0;
		locals[i].lastNanoTime = initialNanoTime;
		locals[i].lastRecalibrateMilliTime = 0;
#if G_DEBUG_THREAD_DUMPING
		locals[i].lastLogTime = 0;
#endif
	}
}

g_clock_local* clockGetLocal()
{
	return _clockGetLocalForProcessor(processorGetCurrentId());
}

void clockWaitForTime(g_tid task, uint64_t wakeTime)
{
	auto waiter = taskingAcquireTask(task);
	if(!waiter)
		return;

	auto local = _clockGetLocalForTask(waiter);
	mutexAcquire(&local->lock);

	_clockRemoveWaiter(local, waiter);

	waiter->timeoutWait.next = nullptr;
	waiter->timeoutWait.wakeTime = wakeTime;
	waiter->timeoutWait.active = true;

	if(local->waiters)
	{
		g_task* prev = nullptr;
		g_task* entry = local->waiters;
		while(entry)
		{
			// Insert in middle
			if(entry->timeoutWait.wakeTime > wakeTime)
			{
				waiter->timeoutWait.next = entry;
				if(prev)
					prev->timeoutWait.next = waiter;
				else
					local->waiters = waiter;
				break;
			}
			prev = entry;
			entry = entry->timeoutWait.next;

			// Insert at tail
			if(!entry)
				prev->timeoutWait.next = waiter;
		}
	}
	else
	{
		// Insert at head
		local->waiters = waiter;
	}

	mutexRelease(&local->lock);
	taskingReleaseTask(waiter);
}

void clockWakeWaiters(g_clock_local* local)
{
	while(local->waiters && local->time >= local->waiters->timeoutWait.wakeTime)
	{
		g_task* task = local->waiters;
		local->waiters = task->timeoutWait.next;
		task->timeoutWait.next = nullptr;
		task->timeoutWait.wakeTime = 0;
		task->timeoutWait.active = false;
		taskingWake(task);
	}
}

void clockUpdateTime(g_clock_local* local)
{
	if(hpetIsAvailable())
	{
		// Timer interrupts can coalesce while interrupts are masked. Advance by
		// elapsed hardware time so sleep deadlines do not stretch under load.
		uint64_t elapsedNanos = hpetGetNanos() - local->lastNanoTime;
		uint64_t elapsedMillis = elapsedNanos / 1000000ULL;
		local->time += elapsedMillis;
		local->lastNanoTime += elapsedMillis * 1000000ULL;
	}
	else
	{
		local->time += (1000 / G_TIMER_FREQUENCY);
	}
}

void clockUpdate()
{
	auto local = clockGetLocal();
	mutexAcquire(&local->lock);
	clockUpdateTime(local);
	clockWakeWaiters(local);
	mutexRelease(&local->lock);
}

void clockUnwaitForTime(g_task* waiter)
{
	auto local = _clockGetLocalForTask(waiter);
	mutexAcquire(&local->lock);
	_clockRemoveWaiter(local, waiter);
	mutexRelease(&local->lock);
}

void clockUnwaitForTime(g_tid task)
{
	auto waiter = taskingAcquireTask(task);
	if(waiter)
		clockUnwaitForTime(waiter);
	taskingReleaseTask(waiter);
}

bool clockHasTimedOut(g_tid task)
{
	auto waiter = taskingAcquireTask(task);
	if(!waiter)
		return true;

	auto local = _clockGetLocalForTask(waiter);
	mutexAcquire(&local->lock);
	bool timeout = !waiter->timeoutWait.active || local->time >= waiter->timeoutWait.wakeTime;
	mutexRelease(&local->lock);
	taskingReleaseTask(waiter);
	return timeout;
}
