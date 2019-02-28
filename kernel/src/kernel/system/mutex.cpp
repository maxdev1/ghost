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

#include "shared/logger/logger.hpp"

void mutexAcquire(g_mutex* mutex)
{
	mutexAcquire(mutex, true);
}

void mutexAcquire(g_mutex* mutex, bool increaseCount)
{
#if G_DEBUG_LOCKS_DEADLOCKING
	uint32_t deadlockCounter = 0;
#endif

	while(!__sync_bool_compare_and_swap(mutex, 0, 1))
	{
		asm("pause");
#if G_DEBUG_LOCKS_DEADLOCKING
		++deadlockCounter;
		if(deadlockCounter > 10000000)
		{
			loggerPrintPlain("deadlock");
		}
#endif
	}

	if(increaseCount)
		__sync_fetch_and_add(&taskingGetLocal()->locksHeld, 1);
}

void mutexRelease(g_mutex* mutex)
{
	mutexRelease(mutex, true);
}

void mutexRelease(g_mutex* mutex, bool decreaseCount)
{
	if(*mutex == 0)
	{
		return;
	}
	*mutex = 0;

	if(decreaseCount)
		__sync_fetch_and_sub(&taskingGetLocal()->locksHeld, 1);
}

bool mutexIsAcquired(g_mutex* mutex)
{
	return *mutex;
}

