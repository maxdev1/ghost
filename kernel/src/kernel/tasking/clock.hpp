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

#include "kernel/filesystem/filesystem.hpp"
#include <ghost/tasks/types.h>

struct g_clock_waiter
{
	g_tid task;
	uint64_t wakeTime;
	g_clock_waiter* next;
};

/**
 * Processor local clock information.
 */
struct g_clock_local
{
	g_clock_waiter* waiters;
	g_mutex lock;

	/**
	 * Approximation of milliseconds that this processor has run.
	 */
	uint64_t time;
};

/**
 * Initializes the clock.
 */
void clockInitialize();

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

/**
 * @returns true when the wake-up time for this task was reached or the queue entry removed.
 */
bool clockHasTimedOut(g_tid task);

#endif
