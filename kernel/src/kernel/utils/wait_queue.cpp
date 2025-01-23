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

#include "kernel/utils/wait_queue.hpp"
#include "kernel/memory/heap.hpp"

void waitQueueAdd(g_wait_queue_entry** queue, g_tid task)
{
	g_wait_queue_entry* waiter = (g_wait_queue_entry*) heapAllocate(sizeof(g_wait_queue_entry));
	waiter->task = task;
	waiter->next = *queue;
	*queue = waiter;
}

void waitQueueRemove(g_wait_queue_entry** queue, g_tid task)
{
	g_wait_queue_entry* prev = nullptr;
	g_wait_queue_entry* waiter = *queue;
	while(waiter)
	{
		if(waiter->task == task)
		{
			auto next = waiter->next;
			if(prev)
				prev->next = next;
			else
				*queue = next;

			heapFree(waiter);
			break;
		}
		prev = waiter;
		waiter = waiter->next;
	}
}

void waitQueueWake(g_wait_queue_entry** queue)
{
	auto waiter = *queue;
	while(waiter)
	{
		g_task* task = taskingGetById(waiter->task);
		if(task && task->status == G_TASK_STATUS_WAITING)
			task->status = G_TASK_STATUS_RUNNING;

		auto next = waiter->next;
		heapFree(waiter);
		waiter = next;
	}
	*queue = nullptr;
}
