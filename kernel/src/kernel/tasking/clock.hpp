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

#include "ghost/types.h"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/tasking/tasking.hpp"

struct g_clock_waiter
{
	g_tid task;
	uint32_t wakeTime;
	g_clock_waiter* next;
};

/**
 * Initializes the clock.
 */
void clockInitialize();

/**
 * Puts the task to sleep and makes it wake up at the given wake time.
 */
void clockWakeAt(g_tid task, uint64_t wakeTime);

/**
 * Called when the local time has changed. Wakes tasks at the given time.
 */
void clockUpdate();

/**
 * Removes the task from the wake list.
 */
void clockUnsetAlarm(g_tid task);

/**
 * Returns whether this alarm has already timed out.
 */
bool clockHasTimedOut(g_tid task);

#endif
