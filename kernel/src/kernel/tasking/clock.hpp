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

#ifndef __KERNEL_CLOCK__
#define __KERNEL_CLOCK__

#include "kernel/system/mutex.hpp"
#include "kernel/build_config.hpp"
#include <ghost/tasks/types.h>

struct g_task;

/**
 * Number of milliseconds on how often a high-precision clock source should be
 * checked to keep millisecond time accurate.
 */
#define G_CLOCK_RECALIBRATION_INTERVAL   1000

/**
 * Processor local clock information.
 */
struct g_clock_local
{
    g_task* waiters;
    g_mutex lock;

    /**
     * Cached system milliseconds used while servicing this CPU's waiters.
     */
    uint64_t time;

    /**
     * Data used for clock recalibiration
     */
    uint64_t lastNanoTime;
    uint64_t lastRecalibrateMilliTime;

#if G_DEBUG_THREAD_DUMPING
    uint64_t lastLogTime;
#endif
};

/**
 * Selects a hardware reference on the bootstrap processor before LAPIC setup.
 */
void clockInitializeReference();

/**
 * Samples the reference in nanoseconds for same-CPU calibration intervals.
 * Returns false when there is no independent clock.
 */
bool clockReadCalibrationNanos(uint64_t* nanos);

/**
 * Initializes the clock and timed wait queues.
 */
void clockInitialize(int64_t bootSeconds);

/**
 * Reads shared monotonic time in nanoseconds. Without a hardware reference,
 * precision is limited to the periodic bootstrap-processor tick.
 */
uint64_t clockGetNanoseconds(uint32_t processor);

/**
 * Reads shared elapsed milliseconds, including on an idle processor.
 */
uint64_t clockGetMilliseconds();

/**
 * Services due waiters after a wake-up without adding a legacy timer tick.
 */
void clockRefreshWaiters();

/**
 * Configures the owner CPU's timer after scheduling with interrupts disabled.
 * The bootstrap processor keeps periodic ticks for legacy time and IRQ upkeep.
 */
void clockConfigureLocalTimer(bool idle);

/**
 * Returns the processor-local clock structure.
 */
g_clock_local* clockGetLocal();

/**
 * Adds the task to the queue of tasks that are waiting for a specific time. This
 * queue is ordered ascending by the time of wake-up.
 */
void clockWaitForTime(g_tid task, uint64_t wakeTime);

/**
 * Called when the local time has changed. Wakes all tasks on top of the wait queue
 * for which the wake-up time is lower than the current.
 */
void clockUpdate();

/**
 * Removes the task from the wake queue.
 */
void clockUnwaitForTime(g_tid task);
void clockUnwaitForTime(g_task* task);

/**
 * @returns true when the wake-up time for this task was reached or the queue entry removed.
 */
bool clockHasTimedOut(g_tid task);

/**
 * @return the real time as a unix timestamp in seconds
 */
int64_t clockGetWallTime();

#endif
