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
#include "kernel/logger/logger.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/panic.hpp"
#include "kernel/system/configuration.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/timing/hpet.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/scheduler/scheduler.hpp"
#include "kernel/system/interrupts/apic/lapic.hpp"

static g_clock_local* locals = nullptr;
static bool hypervisorClock;
static uint32_t vmwareTscKhz;
static uint64_t vmwareTscOrigin;
static uint64_t vmwareLastNanos;
struct g_clock_reference_page
{
	uint32_t sequence;
	uint32_t reserved;
	uint64_t scale;
	int64_t offset;
};
static g_clock_reference_page* referencePage;

static bool clockReferenceAvailable()
{
	return hypervisorClock || vmwareTscKhz || hpetIsAvailable();
}

static uint64_t clockReadHypervisorCounter()
{
	if(referencePage)
	{
		for(;;)
		{
			const auto sequence = memoryAtomicLoad(&referencePage->sequence, G_MEMORY_ORDER_ACQUIRE);
			if(!sequence) break;
			const auto tsc = processorReadTsc();
			const auto scale = memoryAtomicLoad(&referencePage->scale, G_MEMORY_ORDER_RELAXED);
			const auto offset = memoryAtomicLoad(&referencePage->offset, G_MEMORY_ORDER_RELAXED);
			// x86 orders loads; keep the compiler from moving the data reads past
			// the final sequence check during a hypervisor scale/offset update.
			asm volatile("" ::: "memory");
			if(sequence == memoryAtomicLoad(&referencePage->sequence, G_MEMORY_ORDER_ACQUIRE))
				return (uint64_t) (((unsigned __int128) tsc * scale) >> 64) + offset;
		}
	}
	uint32_t low, high;
	processorReadMsr(0x40000020, &low, &high);
	return ((uint64_t) high << 32) | low;
}

static uint64_t clockReadVmwareNanos()
{
	const uint64_t tsc = processorReadTsc();
	if(tsc <= vmwareTscOrigin) return 0;
	const uint64_t ticks = tsc - vmwareTscOrigin;
	// Split before multiplying so even years of TSC ticks convert without
	// overflowing the intermediate product or requiring a 128-bit division.
	return (ticks / vmwareTscKhz) * 1000000ULL +
	       ((ticks % vmwareTscKhz) * 1000000ULL) / vmwareTscKhz;
}

bool clockReadCalibrationNanos(uint64_t* nanos)
{
	// Hv#1's partition-wide reference time has fixed 100 ns units.
	if(hypervisorClock)
		*nanos = clockReadHypervisorCounter() * 100ULL;
	else if(vmwareTscKhz)
		*nanos = clockReadVmwareNanos();
	else if(hpetIsAvailable())
		*nanos = hpetGetNanos();
	else
		return false;
	return true;
}

static uint64_t clockReadReferenceNanos()
{
	uint64_t nanos = 0;
	clockReadCalibrationNanos(&nanos);
	if(!vmwareTscKhz) return nanos;

	// The VMware virtual TSCs share a rate but may have small fixed offsets.
	// Publish a single monotonically increasing clock to migrating readers.
	auto last = memoryAtomicLoad(&vmwareLastNanos, G_MEMORY_ORDER_RELAXED);
	while(nanos > last)
	{
		if(memoryAtomicCompareExchange(&vmwareLastNanos, &last, nanos, false,
		                               G_MEMORY_ORDER_RELAXED, G_MEMORY_ORDER_RELAXED))
			return nanos;
	}
	return last;
}

/**
 * Tracks the real time, based on Limines provided unix timestamp
 */
static int64_t wallClockUnixSeconds = 0;
static uint32_t wallClockProcessor;
static uint64_t wallClockStartNanos = 0;

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

void clockInitializeReference()
{
	hypervisorClock = processorHasHypervFeature(1u << 1);
	referencePage = nullptr;
	vmwareTscKhz = !hypervisorClock && !hpetIsAvailable() ? processorGetVmwareTscKhz() : 0;
	if(vmwareTscKhz)
	{
		vmwareTscOrigin = processorReadTsc();
		vmwareLastNanos = 0;
		logInfo("%! using VMware TSC at %i kHz", "clock", vmwareTscKhz);
	}
	else if(!clockReferenceAvailable())
		logInfo("%! using periodic bootstrap processor ticks", "clock");
}

void clockInitialize(int64_t bootSeconds)
{
	if(hypervisorClock)
	{
		logInfo("%! using Hyper-V reference counter", "clock");
		if(processorHasHypervFeature(1u << 9))
		{
			auto page = memoryAllocateKernel(1);
			if(page)
			{
				memorySetBytes((void*) page, 0, G_PAGE_SIZE);
				uint32_t low, high;
				processorReadMsr(0x40000021, &low, &high);
				const uint64_t control = pagingVirtualToPhysical(page) | (low & 0xffe) | 1;
				processorWriteMsr(0x40000021, control, control >> 32);
				referencePage = (g_clock_reference_page*) page;
				logInfo("%! enabled shared Hyper-V reference TSC page", "clock");
			}
		}
	}
	uint32_t numProcs = processorGetNumberOfProcessors();
	locals = (g_clock_local*) heapAllocate(sizeof(g_clock_local) * numProcs);
	const uint64_t initialNanoTime = clockReadReferenceNanos();

	wallClockUnixSeconds = bootSeconds;
	wallClockProcessor = processorGetCurrentId();
	wallClockStartNanos = initialNanoTime;

	if(numProcs > 1)
		logInfo("%! active scheduling %i Hz; idle secondary timers use deadlines", "clock", G_TIMER_FREQUENCY);

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

g_clock_local* clockGetLocal() { return _clockGetLocalForProcessor(processorGetCurrentId()); }

uint64_t clockGetMilliseconds()
{
	if(clockReferenceAvailable())
	{
		const auto now = clockReadReferenceNanos();
		return now >= wallClockStartNanos ? (now - wallClockStartNanos) / 1000000ULL : 0;
	}

	return memoryAtomicLoad(&_clockGetLocalForProcessor(wallClockProcessor)->time, G_MEMORY_ORDER_ACQUIRE);
}

uint64_t clockGetNanoseconds(uint32_t processor)
{
	if(clockReferenceAvailable())
		return clockReadReferenceNanos();

	return clockGetMilliseconds() * 1000000ULL;
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
	if(waiter->assignment)
		schedulerRequestReschedule(waiter->assignment->processor);
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
	if(clockReferenceAvailable())
	{
		// Timer interrupts can coalesce while interrupts are masked. Advance by
		// elapsed hardware time so sleep deadlines do not stretch under load.
		const uint64_t now = clockReadReferenceNanos();
		if(now <= local->lastNanoTime) return;
		uint64_t elapsedNanos = now - local->lastNanoTime;
		uint64_t elapsedMillis = elapsedNanos / 1000000ULL;
		memoryAtomicAddFetch(&local->time, elapsedMillis, G_MEMORY_ORDER_RELEASE);
		local->lastNanoTime += elapsedMillis * 1000000ULL;
	}
	else
	{
		memoryAtomicAddFetch(&local->time, 1000 / G_TIMER_FREQUENCY, G_MEMORY_ORDER_RELEASE);
	}
}

void clockUpdate()
{
	auto local = clockGetLocal();
	mutexAcquire(&local->lock);
	if(clockReferenceAvailable() || processorGetCurrentId() == wallClockProcessor)
		clockUpdateTime(local);
	else
		memoryAtomicStore(&local->time, clockGetMilliseconds(), G_MEMORY_ORDER_RELEASE);
	clockWakeWaiters(local);
	mutexRelease(&local->lock);
}

void clockRefreshWaiters()
{
	auto local = clockGetLocal();
	mutexAcquire(&local->lock);
	if(clockReferenceAvailable())
		clockUpdateTime(local);
	else if(processorGetCurrentId() != wallClockProcessor)
		memoryAtomicStore(&local->time, clockGetMilliseconds(), G_MEMORY_ORDER_RELEASE);
	clockWakeWaiters(local);
	mutexRelease(&local->lock);
}

void clockConfigureLocalTimer(bool idle)
{
	if(!lapicIsAvailable())
		return;

	idle = idle && processorGetCurrentId() != wallClockProcessor;
	uint64_t delay = 0;
	if(idle)
	{
		auto local = clockGetLocal();
		mutexAcquire(&local->lock);
		if(local->waiters)
		{
			const auto now = clockGetMilliseconds();
			const auto deadline = local->waiters->timeoutWait.wakeTime;
			delay = deadline > now ? deadline - now : 1;
		}
		mutexRelease(&local->lock);
	}

	lapicConfigureTimer(idle, delay);
}

void clockUnwaitForTime(g_task* waiter)
{
	auto local = _clockGetLocalForTask(waiter);
	mutexAcquire(&local->lock);
	_clockRemoveWaiter(local, waiter);
	mutexRelease(&local->lock);
	if(waiter->assignment)
		schedulerRequestReschedule(waiter->assignment->processor);
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
	bool timeout = !waiter->timeoutWait.active || clockGetMilliseconds() >= waiter->timeoutWait.wakeTime;
	mutexRelease(&local->lock);
	taskingReleaseTask(waiter);
	return timeout;
}

int64_t clockGetWallTime()
{
	if(wallClockUnixSeconds <= 0)
		return -1;

	uint64_t now = clockGetNanoseconds(wallClockProcessor);
	if(now < wallClockStartNanos)
		return -1;

	uint64_t elapsed = (now - wallClockStartNanos) / 1000000000ULL;
	if(elapsed > uint64_t(INT64_MAX - wallClockUnixSeconds))
		return -1;
	return wallClockUnixSeconds + int64_t(elapsed);
}
