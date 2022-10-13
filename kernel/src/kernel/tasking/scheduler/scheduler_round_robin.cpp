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

#include "kernel/memory/heap.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/atoms.hpp"
#include "kernel/tasking/scheduler/scheduler.hpp"
#include "shared/logger/logger.hpp"

#if G_DEBUG_THREAD_DUMPING
#include "kernel/tasking/clock.hpp"
#endif

#define G_DEBUG_LOG_PAUSE 5000

void schedulerInitializeLocal()
{
}

void schedulerPrepareEntry(g_schedule_entry* entry)
{
}

g_schedule_entry* schedulerGetNextTask(g_tasking_local* local)
{
	g_schedule_entry* entry = local->scheduling.list;
	if(local->scheduling.current == local->scheduling.idleTask)
	{
		return entry;
	}

	while(entry)
	{
		if(entry->task == local->scheduling.current)
		{
			break;
		}
		entry = entry->next;
	}

	if(entry)
	{
		entry = entry->next;
	}
	if(!entry)
	{
		entry = local->scheduling.list;
	}
	return entry;
}

void schedulerSchedule(g_tasking_local* local)
{
	mutexAcquire(&local->lock);

	if(!local->scheduling.current)
	{
		local->scheduling.current = local->scheduling.list->task;
		mutexRelease(&local->lock);
		return;
	}

	g_schedule_entry* start = schedulerGetNextTask(local);
	g_schedule_entry* entry = start;
	for(;;)
	{
		g_task* task = entry->task;
		if(task->status == G_THREAD_STATUS_RUNNING)
		{
			local->scheduling.current = task;
			local->scheduling.current->statistics.timesScheduled++;
			break;
		}

		entry = entry->next;
		if(!entry)
		{
			entry = local->scheduling.list;
		}

		if(entry == start)
		{
			local->scheduling.current = local->scheduling.idleTask;
			local->scheduling.idleTask->statistics.timesScheduled++;
			break;
		}
	}
	mutexRelease(&local->lock);

#if G_DEBUG_THREAD_DUMPING
	static int lastLogTime = 0;
	if((clockGetLocal()->time - lastLogTime) > G_DEBUG_LOG_PAUSE)
	{
		lastLogTime = clockGetLocal()->time;
		schedulerDump();
	}
#endif
}

#define USAGE(timesScheduled, timesYielded) ((timesScheduled - timesYielded) / (G_DEBUG_LOG_PAUSE / 1000))

void schedulerDump()
{
	g_tasking_local* local = taskingGetLocal();
	mutexAcquire(&local->lock);

	logInfo("%! dump @%i", "sched", processorGetCurrentId());
	g_schedule_entry* entry = local->scheduling.list;
	while(entry)
	{
		const char* taskState;
		if(entry->task->status == G_THREAD_STATUS_RUNNING)
		{
			taskState = "";
		}
		else if(entry->task->status == G_THREAD_STATUS_DEAD)
		{
			taskState = " [dead]";
		}
		else if(entry->task->status == G_THREAD_STATUS_WAITING)
		{
			taskState = " [waiting]";
		}

		if(entry->task->status != G_THREAD_STATUS_DEAD)
		{
			logInfo("%# (%i:%i)%s usage: %i", entry->task->process->id, entry->task->id, taskState, USAGE(entry->task->statistics.timesScheduled, entry->task->statistics.timesYielded));
			entry->task->statistics.timesScheduled = 0;
			entry->task->statistics.timesYielded = 0;
		}
		entry = entry->next;
	}

	g_task* idle = local->scheduling.idleTask;
	logInfo("%# (%i:%i) idle, usage: %i", idle->process->id, idle->id, USAGE(idle->statistics.timesScheduled, idle->statistics.timesYielded));
	idle->statistics.timesScheduled = 0;
	idle->statistics.timesYielded = 0;

	mutexRelease(&local->lock);
}
