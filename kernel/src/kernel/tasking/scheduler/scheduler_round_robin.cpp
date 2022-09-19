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
#include "kernel/tasking/scheduler.hpp"
#include "kernel/tasking/wait.hpp"
#include "shared/logger/logger.hpp"

void schedulerInitializeLocal()
{
    taskingGetLocal()->scheduling.round = 1;
}

void schedulerNewTimeSlot()
{
    taskingGetLocal()->scheduling.round++;
}

void schedulerPrepareEntry(g_schedule_entry* entry)
{
    entry->schedulerRound = 0;
}

g_schedule_entry* schedulerGetCurrentEntry(g_tasking_local* local)
{
    g_schedule_entry* entry = local->scheduling.list;
    while(entry)
    {
        if(entry->task == local->scheduling.current)
        {
            break;
        }
        entry = entry->next;
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

    g_schedule_entry* start = schedulerGetCurrentEntry(local);
    if(!start)
    {
        start = local->scheduling.list;
    }

    bool busy = false;
    g_schedule_entry* entry = start;
    for(;;)
    {
        if(entry->schedulerRound < local->scheduling.round)
        {
            entry->schedulerRound = local->scheduling.round;

            g_task* task = entry->task;
            if(task->status == G_THREAD_STATUS_RUNNING || task->status == G_THREAD_STATUS_WAITING)
            {
                local->scheduling.current = task;

                busy = true;
                break;
            }
        }

        entry = entry->next;
        if(!entry)
        {
            entry = local->scheduling.list;
        }
        if(entry == start)
            break;
    }

    if(busy)
    {
        local->scheduling.current->timesScheduled++;
    }
    else
    {
        local->scheduling.current = local->scheduling.idleTask;
    }

    static int times = 0;
    if(times++ % 10000 == 0)
        schedulerDump();

    mutexRelease(&local->lock);
}

void schedulerDump()
{
    g_tasking_local* local = taskingGetLocal();
    mutexAcquire(&local->lock);

    logInfo("%! dump (task count: %i)", "sched", local->scheduling.taskCount);
    g_schedule_entry* entry = local->scheduling.list;
    while(entry)
    {
        const char* taskState;
        if(entry->task->status == G_THREAD_STATUS_RUNNING)
        {
            taskState = "running";
        }
        else if(entry->task->status == G_THREAD_STATUS_UNUSED)
        {
            taskState = "unused";
        }
        else if(entry->task->status == G_THREAD_STATUS_DEAD)
        {
            taskState = "dead";
        }
        else if(entry->task->status == G_THREAD_STATUS_WAITING)
        {
            taskState = "waiting";
        }

        logInfo("%# process: %i, task: %i, status: %s, tSch: %i, tYld: %i, time: %i, round: %i",
                entry->task->process->id, entry->task->id, taskState, entry->task->timesScheduled, entry->task->timesYielded,
                entry->task->timesScheduled - entry->task->timesYielded, entry->schedulerRound);
        entry = entry->next;
    }

    mutexRelease(&local->lock);
}
