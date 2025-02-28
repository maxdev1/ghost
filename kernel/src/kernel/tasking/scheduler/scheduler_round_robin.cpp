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

#include "kernel/tasking/clock.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/tasking_directory.hpp"

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

	// Check if there is a global "preferred task" to do next
	if(preferredTask != G_TID_NONE)
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
	if(processorGetCurrentId() == 0 && (clockGetLocal()->time - clockGetLocal()->lastLogTime) > G_DEBUG_LOG_PAUSE)
	{
		clockGetLocal()->lastLogTime = clockGetLocal()->time;
		schedulerDump();
	}
#endif
}

#define USAGE(timesScheduled, timesYielded) ((timesScheduled - timesYielded) / (G_DEBUG_LOG_PAUSE / 1000))

void schedulerDump()
{
	// This only works because the interrupt comes in on CPU0:
	logInfo("%! printing task dump:", "scheduler");
	auto firstLocal = taskingGetLocal();
	auto firstClock = clockGetLocal();
	for(int i = 0; i < processorGetNumberOfProcessors(); i++)
	{
		auto local = &firstLocal[i];
		auto clock = &firstClock[i];
		mutexAcquire(&local->lock);

		logInfo("%# processor %i: time %i", i, (uint32_t) clock->time);
		g_schedule_entry* entry = local->scheduling.list;
		while(entry)
		{
			auto task = entry->task;
			const char* taskState;
			if(task->status == G_TASK_STATUS_RUNNING)
			{
				taskState = "";
			}
			else if(task->status == G_TASK_STATUS_DEAD)
			{
				taskState = " [dead]";
			}
			else if(task->status == G_TASK_STATUS_WAITING)
			{
				taskState = " [waiting]";
			}
			else
			{
				taskState = "?";
			}

			if(task->status != G_TASK_STATUS_DEAD)
			{
				auto identifier = taskingDirectoryGetIdentifier(task->id);
				logInfo("%# - (%i:%i) usage: %i, %s %s %s", task->process->id, task->id,
				        USAGE(task->statistics.timesScheduled, task->statistics.timesYielded),
				        identifier == nullptr ? "" : identifier, taskState,
				        task->status == G_TASK_STATUS_WAITING ? task->waitsFor : "");
				task->statistics.timesScheduled = 0;
				task->statistics.timesYielded = 0;
			}

			entry = entry->next;
		}

		g_task* idle = local->scheduling.idleTask;
		logInfo("%# - (%i:%i) usage: %i, idle",
		        idle->process->id, idle->id,
		        USAGE(idle->statistics.timesScheduled, idle->statistics.timesYielded)
				);
		idle->statistics.timesScheduled = 0;
		idle->statistics.timesYielded = 0;

		mutexRelease(&local->lock);
	}
}
