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

#ifndef __SYSTEM_SMP_MUTEX__
#define __SYSTEM_SMP_MUTEX__

#include "ghost/stdint.h"

struct g_mutex
{
	volatile int initialized = 0;
	volatile int lock = 0;
	int depth = 0;
	uint32_t owner = -1;
};

/**
 * Initializes the mutex.
 */
void mutexInitialize(g_mutex* mutex);

/**
 * Acquires the mutex. Increases the lock count for this processor.
 */
void mutexAcquire(g_mutex* mutex);
bool mutexTryAcquire(g_mutex* mutex);

/**
 * Releases the mutex. Decreases the lock count for this processor.
 */
void mutexRelease(g_mutex* mutex);

/**
 * Acquires the mutex.
 *
 * The smp parameter decides if the lock count for this processor should be increased.
 */
void mutexAcquire(g_mutex* mutex, bool smp);
bool mutexTryAcquire(g_mutex* mutex, bool smp);

/**
 * Releases the mutex.
 *
 * The smp parameter decides if the lock count for this processor should be decreased.
 */
void mutexRelease(g_mutex* mutex, bool smp);

#endif
