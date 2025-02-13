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
#include "kernel/tasking/tasking.hpp"

void waitQueueInitialize(g_wait_queue* queue)
{
	mutexInitializeTask(&queue->lock);
	queue->head = nullptr;
}

void waitQueueAdd(g_wait_queue* queue, g_tid task)
{
	mutexAcquire(&queue->lock);

	auto entry = (g_wait_queue_entry*) heapAllocate(sizeof(g_wait_queue_entry));
	entry->task = task;
	entry->next = queue->head;
	queue->head = entry;

	mutexRelease(&queue->lock);
}

void waitQueueRemove(g_wait_queue* queue, g_tid task)
{
	mutexAcquire(&queue->lock);

	g_wait_queue_entry* prev = nullptr;
	g_wait_queue_entry* waiter = queue->head;
	while(waiter)
	{
		if(waiter->task == task)
		{
			auto next = waiter->next;
			if(prev)
				prev->next = next;
			else
				queue->head = next;

			heapFree(waiter);
			break;
		}
		prev = waiter;
		waiter = waiter->next;
	}

	mutexRelease(&queue->lock);
}

void waitQueueWake(g_wait_queue* queue)
{
	mutexAcquire(&queue->lock);

	auto waiter = queue->head;
	while(waiter)
	{
		g_task* task = taskingGetById(waiter->task);
		if(task)
		{
			mutexAcquire(&task->lock);
			if(task->status == G_TASK_STATUS_WAITING)
				task->status = G_TASK_STATUS_RUNNING;
			mutexRelease(&task->lock);
		}

		auto next = waiter->next;
		heapFree(waiter);
		waiter = next;
	}
	queue->head = nullptr;

	mutexRelease(&queue->lock);
}
