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

#ifndef __KERNEL_SCHEDULER__
#define __KERNEL_SCHEDULER__

#include "kernel/tasking/tasking.hpp"

/**
 * Initializes the scheduler locally.
 */
void schedulerInitializeLocal();

/**
 * Called when a new time slot has started.
 */
void schedulerNewTimeSlot();

/**
 * Prepares a new task entry for scheduling.
 */
void schedulerPrepareEntry(g_schedule_entry* entry);

/**
 * Schedules to the next task.
 */
void schedulerSchedule(g_tasking_local* local);

/**
 * Prefers the given task when next schedule is requested.
 */
void schedulerPleaseSchedule(g_task* task);

#endif
