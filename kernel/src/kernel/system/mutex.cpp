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

#include "kernel/system/mutex.hpp"

#include "shared/logger/logger.hpp"

void mutexAcquire(g_mutex* mutex)
{
#if G_DEBUG_LOCKS_DEADLOCKING
	uint32_t deadlockCounter = 0;
#endif

	while(!__sync_bool_compare_and_swap(mutex, 0, 1))
	{
		asm("pause");
#if G_DEBUG_LOCKS_DEADLOCKING
		++deadlockCounter;
		if (deadlockCounter > 100000000)
		{
			logInfo("%! looks like a deadlock", "lock");
		}
#endif
	}
}

void mutexRelease(g_mutex* mutex)
{
	*mutex = 0;
}

bool mutexIsLocked(g_mutex* mutex)
{
	return *mutex;
}
