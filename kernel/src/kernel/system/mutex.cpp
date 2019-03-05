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

#include "shared/system/mutex.hpp"

#include "kernel/tasking/tasking.hpp"
#include "kernel/kernel.hpp"

#include "shared/logger/logger.hpp"

void mutexInitialize(g_mutex* mutex)
{
	if(__sync_bool_compare_and_swap(&mutex->initialized, 0, 1))
	{
		mutex->initialized = 1;
		mutex->mutex = 0;
		mutex->depth = 0;
		mutex->owner = -1;
	}
}

void mutexAcquire(g_mutex* mutex)
{
	mutexAcquire(mutex, true);
}

void mutexAcquire(g_mutex* mutex, bool increaseCount)
{
	mutexInitialize(mutex);

#if G_DEBUG_LOCKS_DEADLOCKING
	uint32_t deadlockCounter = 0;
#endif

	while(!__sync_bool_compare_and_swap(&mutex->mutex, 0, 1))
	{
		if(mutex->owner == processorGetCurrentId())
		{
			++mutex->depth;
			return;
		}
		asm("pause");

#if G_DEBUG_LOCKS_DEADLOCKING
		++deadlockCounter;
		if (deadlockCounter > 100000000)
		{
			logInfo("%! looks like a deadlock", "rlock");
		}
#endif

	}
	mutex->owner = processorGetCurrentId();
	mutex->depth = 0;

	if(increaseCount)
		__sync_fetch_and_add(&taskingGetLocal()->locksHeld, 1);
}

void mutexRelease(g_mutex* mutex)
{
	mutexRelease(mutex, true);
}

void mutexRelease(g_mutex* mutex, bool decreaseCount)
{
	if(!mutex->initialized)
		kernelPanic("%! tried to use uninitialized mutex", "mutex");

	if(mutex->depth > 0)
	{
		--mutex->depth;
		return;
	}

	mutex->owner = -1;
	mutex->mutex = 0;

	if(decreaseCount)
		__sync_fetch_and_sub(&taskingGetLocal()->locksHeld, 1);
}

bool mutexIsAcquired(g_mutex* mutex)
{
	return mutex->mutex;
}

