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
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/system/interrupts/interrupts.hpp"

void taskingCleanupThread()
{
	g_tasking_local* local = taskingGetLocal();
	g_task* self = taskingGetCurrentTask();
	for(;;)
	{
		// Find and remove dead tasks from local scheduling list
		mutexAcquire(&local->lock);

		g_schedule_entry* deadList = nullptr;
		g_schedule_entry* entry = local->scheduling.list;
		g_schedule_entry* previous = nullptr;
		while(entry)
		{
			g_schedule_entry* next = entry->next;

			auto task = entry->task;
			mutexAcquire(&task->lock);

			if(task->status == G_TASK_STATUS_DEAD)
			{
				if(previous)
					previous->next = next;
				else
					local->scheduling.list = next;

				entry->next = deadList;
				deadList = entry;
			}
			else
			{
				previous = entry;
			}
			entry = next;

			mutexRelease(&task->lock);
		}

		mutexRelease(&local->lock);

		// Remove each task
		while(deadList)
		{
			g_schedule_entry* next = deadList->next;
			taskingDestroyTask(deadList->task);
			heapFree(deadList);
			deadList = next;
		}

		// Sleep for some time
		mutexAcquire(&self->lock);
		clockWaitForTime(self->id, clockGetLocal()->time + 3000);
		self->status = G_TASK_STATUS_WAITING;
		mutexRelease(&self->lock);
		taskingYield();
	}
}
