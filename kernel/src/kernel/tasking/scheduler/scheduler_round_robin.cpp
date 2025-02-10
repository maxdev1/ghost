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

#include "kernel/tasking/scheduler/scheduler.hpp"
#include "shared/logger/logger.hpp"

#if G_DEBUG_THREAD_DUMPING
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#endif

#define G_DEBUG_LOG_PAUSE 5000

/**
 * The "preferred task" is just a hint to scheduling that it would make sense to
 * switch to a specific task. It is checked on all cores.
 */
g_tid preferredTask;

void schedulerInitializeLocal()
{
	preferredTask = G_TID_NONE;
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

	// // Check if there is a global "preferred task" to do next
	// TODO: This causes rare deadlocks...
	if(false && preferredTask != G_TID_NONE)
	{
		while(entry)
		{
			if(entry->task->id == preferredTask)
			{
				preferredTask = G_TID_NONE;
				return entry;
			}
			entry = entry->next;
		}
	}

	// Otherwise just find our current tasks entry and choose the next one
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

void schedulerPrefer(g_tid task)
{
	preferredTask = task;
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

		bool done = false;

		mutexAcquire(&task->lock);
		if(task->status == G_TASK_STATUS_RUNNING)
		{
			local->scheduling.current = task;
			local->scheduling.current->statistics.timesScheduled++;
			done = true;
		}
		else
		{
			entry = entry->next;
			if(!entry)
			{
				entry = local->scheduling.list;
			}

			if(entry == start)
			{
				local->scheduling.current = local->scheduling.idleTask;
				local->scheduling.idleTask->statistics.timesScheduled++;
				done = true;
			}
		}
		mutexRelease(&task->lock);

		if(done)
			break;
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

#if G_DEBUG_THREAD_DUMPING
void schedulerDump()
{
	g_tasking_local* local = taskingGetLocal();
	mutexAcquire(&local->lock);

	auto processorId = processorGetCurrentId();
	logInfo("%# time on %i: %i", processorId, (uint32_t) clockGetLocal()->time);
	g_schedule_entry* entry = local->scheduling.list;
	while(entry)
	{
		const char* taskState;
		if(entry->task->status == G_TASK_STATUS_RUNNING)
		{
			taskState = "";
		}
		else if(entry->task->status == G_TASK_STATUS_DEAD)
		{
			taskState = " [dead]";
		}
		else if(entry->task->status == G_TASK_STATUS_WAITING)
		{
			taskState = " [waiting]";
		}

		if(entry->task->status != G_TASK_STATUS_DEAD)
		{
			auto ident = taskingDirectoryGetIdentifier(entry->task->id);
			logInfo("%# @%i (%i:%i:%s)%s usage: %i", processorId, entry->task->process->id, entry->task->id,
			        ident == nullptr?"":ident, taskState,
			        USAGE(entry->task->statistics.timesScheduled, entry->task->statistics.timesYielded));
			entry->task->statistics.timesScheduled = 0;
			entry->task->statistics.timesYielded = 0;
		}
		entry = entry->next;
	}

	g_task* idle = local->scheduling.idleTask;
	logInfo("%# (%i:%i) idle, usage: %i", idle->process->id, idle->id,
	        USAGE(idle->statistics.timesScheduled, idle->statistics.timesYielded));
	idle->statistics.timesScheduled = 0;
	idle->statistics.timesYielded = 0;

	mutexRelease(&local->lock);
}
#endif
