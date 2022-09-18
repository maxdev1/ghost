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

#include "kernel/tasking/wait.hpp"

#include "kernel/memory/heap.hpp"
#include "shared/logger/logger.hpp"

void waitSleep(g_task* task, uint64_t milliseconds)
{
    uint64_t wakeTime = taskingGetLocal()->time + milliseconds;
    task->status = G_THREAD_STATUS_WAITING;
    for(;;)
    {
        if(taskingGetLocal()->time > wakeTime)
        {
            break;
        }
        taskingYield();
    }
    task->status = G_THREAD_STATUS_RUNNING;
}

void waitForFile(g_task* task, g_fs_node* file,
                 bool (*waitResolverFromDelegate)(g_fs_virt_id))
{
    task->status = G_THREAD_STATUS_WAITING;
    for(;;)
    {
        if(waitResolverFromDelegate(file->id))
        {
            break;
        }
        taskingYield();
    }
    task->status = G_THREAD_STATUS_RUNNING;
}

void waitJoinTask(g_task* task, g_tid otherTaskId)
{
    task->status = G_THREAD_STATUS_WAITING;
    for(;;)
    {
        g_task* otherTask = taskingGetById(otherTaskId);
        if(otherTask == 0 || otherTask->status == G_THREAD_STATUS_DEAD ||
           otherTask->status == G_THREAD_STATUS_UNUSED)
        {
            break;
        }
        taskingYield();
    }
    task->status = G_THREAD_STATUS_RUNNING;
}
