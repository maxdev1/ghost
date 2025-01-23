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

#ifndef __KERNEL_USER_MUTEX__
#define __KERNEL_USER_MUTEX__

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/tasking/tasking.hpp"
#include <ghost/tasks/types.h>
#include <ghost/mutex/types.h>

struct g_user_mutex_waiter
{
    g_tid task;
    g_user_mutex_waiter* next;
};

struct g_user_mutex_entry
{
    g_mutex lock;
    int value;

    bool reentrant;
    g_tid owner;

    g_user_mutex_waiter* waiters;
};

typedef uint32_t g_user_mutex_status;
#define G_USER_MUTEX_STATUS_ACQUIRED      ((g_user_mutex_status) 1)
#define G_USER_MUTEX_STATUS_TIMEOUT       ((g_user_mutex_status) 2)
#define G_USER_MUTEX_STATUS_NOT_ACQUIRED  ((g_user_mutex_status) 3)

/**
 * Initializes the mutexes.
 */
void userMutexInitialize();

/**
 * Creates a new mutex that can then be locked with the other functions.
 */
g_user_mutex userMutexCreate(bool reentrant);

/**
 * Destroys a mutex.
 */
void userMutexDestroy(g_user_mutex mutex);

/**
 *
 */
g_user_mutex_status userMutexAcquire(g_task* task, g_user_mutex mutex, uint64_t timeout, bool trying);

/**
 *
 */
g_user_mutex_status userMutexTryAcquire(g_task* task, g_user_mutex mutex);

/**
 * Unlocks the mutex and wakes the next waiting task.
 */
void userMutexRelease(g_user_mutex mutex);

/**
 * Removes the task from a wait queue, for example in case of timeouts.
 */
void userMutexUnwaitForAcquire(g_user_mutex mutex, g_tid task);

/**
 * Adds the task to the wait queue for the mutex.
 */
void userMutexWaitForAcquire(g_user_mutex mutex, g_tid task);

#endif
