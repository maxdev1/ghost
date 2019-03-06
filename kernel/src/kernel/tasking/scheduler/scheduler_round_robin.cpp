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

#include "kernel/tasking/scheduler.hpp"
#include "kernel/memory/heap.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/tasking/wait.hpp"

void schedulerInitializeLocal()
{
	taskingGetLocal()->schedulerRound = 1;
}

void schedulerNewTimeSlot()
{
	taskingGetLocal()->schedulerRound++;
}

void schedulerPrepareEntry(g_schedule_entry* entry)
{
	entry->schedulerRound = 0;
}

/**
 * This scheduler implementation keeps a round counter on the
 * local tasking structure and each schedule entry. A new round
 * starts when a new timeslot starts.
 * 
 * When looking for a new task to schedule, each task in the list
 * is checked once. Only tasks that have not been scheduled in this
 * round are taken into account.
 * 
 * If all tasks are waiting/were already scheduled, the idle task is run.
 */
void schedulerSchedule(g_tasking_local* local)
{
	mutexAcquire(&local->lock);

	if(!local->current)
	{
		local->current = local->scheduleList->task;
		mutexRelease(&local->lock);
		return;
	}

	// Find current task in list
	g_schedule_entry* entry = local->scheduleList;
	while(entry)
	{
		if(entry->task == local->current)
		{
			break;
		}
		entry = entry->next;
	}
	if(!entry)
	{
		entry = local->scheduleList;
	}

	bool switched = false;
	uint32_t max = local->taskCount;
	while(max-- > 0)
	{
		// Select next in list
		entry = entry->next;
		if(!entry)
		{
			entry = local->scheduleList;
		}

		// Check if task was already processed this round
		if(entry->schedulerRound >= local->schedulerRound)
		{
			continue;
		}
		entry->schedulerRound = local->schedulerRound;

		// Running task can be scheduled
		g_task* task = entry->task;
		if(task->status == G_THREAD_STATUS_RUNNING)
		{
			local->current = task;
			switched = true;
			break;
		}

		// Waiting task must be checked
		if(task->status == G_THREAD_STATUS_WAITING && waitTryWake(task))
		{
			local->current = task;
			switched = true;
			break;
		}
	}

	// Nothing to schedule, idle
	if(!switched)
	{
		local->current = local->idleTask;
	}
	
	mutexRelease(&local->lock);
}
