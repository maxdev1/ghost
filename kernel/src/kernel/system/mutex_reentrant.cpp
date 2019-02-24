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

#include "kernel/system/mutex_reentrant.hpp"
#include "kernel/system/processor/processor.hpp"
#include "shared/logger/logger.hpp"

void mutexReentrantAcquire(g_mutex_reentrant* mutex)
{
	mutexAcquire(&mutex->mutex);

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
}

void mutexReentrantRelease(g_mutex_reentrant* mutex)
{
	mutexRelease(&mutex->mutex);
	if(mutex->depth > 0)
	{
		--mutex->depth;
		return;
	}

	mutex->owner = -1;
	mutex->mutex = 0;
}

bool mutexReentrantIsAcquired(g_mutex_reentrant* mutex)
{
	return mutex->mutex;
}

