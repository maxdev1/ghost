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

#ifndef __UTILS_WAITQUEUE__
#define __UTILS_WAITQUEUE__

#include "kernel/system/mutex.hpp"

#include <ghost/tasks/types.h>

struct g_wait_queue;
struct g_wait_queue_entry
{
    g_tid task;
    g_wait_queue_entry* next;
    g_wait_queue* queue;
    g_wait_queue_entry* registryNext;
    g_wait_queue_entry* registryPrevious;
};

struct g_wait_queue
{
    g_wait_queue_entry* head;
};

/**
 * Initializes a wait-queue.
 */
void waitQueueInitialize(g_wait_queue* queue);

/**
 * Adds a task entry to the given wait queue.
 */
void waitQueueAdd(g_wait_queue* queue, g_tid task);

/**
 * Removes the entry for this task id from the wait queue.
 */
void waitQueueRemove(g_wait_queue* queue, g_tid task);

/**
 * Wakes all tasks in the queue.
 */
void waitQueueWake(g_wait_queue* queue);

/** Removes every wait registration held by a task before destroying it. */
void waitQueueRemoveTask(g_tid task);

bool waitQueueIsEmpty(g_wait_queue* queue);

#endif
