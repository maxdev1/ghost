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

#ifndef __SYSTEM_MUTEX__
#define __SYSTEM_MUTEX__

#include "kernel/system/spinlock.hpp"

#include <ghost/stdint.h>

typedef int g_mutex_type;
#define G_MUTEX_TYPE_GLOBAL   ((g_mutex_type) 0)
#define G_MUTEX_TYPE_TASK     ((g_mutex_type) 1)

typedef struct
{
    volatile int initialized;
    g_spinlock lock;

    const char* location;
    g_mutex_type type;
    int depth;
    uint32_t owner;
} __attribute__((packed)) g_mutex;

/**
 * Initializes a task mutex. This mutex will yield until it can be acquired.
 */
void mutexInitializeTask(g_mutex* mutex, const char* location = "unknown");

/**
 * Initializes a global mutex for a critical section. Acquiring any global
 * mutex disables interrupts until the last one is released.
 */
void mutexInitializeGlobal(g_mutex* mutex, const char* location = "unknown");

/**
 * Acquires the mutex. Increases the lock count for this processor.
 */
void mutexAcquire(g_mutex* mutex);

/**
 * Releases the mutex. Decreases the lock count for this processor.
 */
void mutexRelease(g_mutex* mutex);

#endif
