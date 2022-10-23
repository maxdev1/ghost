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
#include "kernel/debug/debug.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/logger/logger.hpp"
#include "shared/panic.hpp"
#include "shared/video/console_video.hpp"

g_spinlock mutexInitializerLock = 0;
#define G_MUTEX_INITIALIZED 0xFEED

#define MUTEX_GUARD                               \
	if(mutex->initialized != G_MUTEX_INITIALIZED) \
		mutexErrorUninitialized(mutex);

void mutexErrorUninitialized(g_mutex* mutex)
{
	panic("%! %i: tried to use uninitialized mutex %h", "mutex", processorGetCurrentId(), mutex);
}

void _mutexInitialize(g_mutex* mutex)
{
	G_SPINLOCK_ACQUIRE(mutexInitializerLock);

	mutex->initialized = G_MUTEX_INITIALIZED;
	mutex->lock = 0;
	mutex->depth = 0;
	mutex->owner = -1;

	G_SPINLOCK_RELEASE(mutexInitializerLock);
}

void mutexAcquire(g_mutex* mutex)
{
	MUTEX_GUARD;

	uint32_t owner = processorGetCurrentId();

#if G_DEBUG_MUTEXES
	int dead = 0;
#endif

	while(!mutexTryAcquire(mutex, owner))
	{
#if G_DEBUG_MUTEXES
		dead++;
		if(dead > 10000)
		{
			logInfo("%i likely deadlocked @%x (owner: %i, depth: %i)", owner, mutex, mutex->owner, mutex->depth);
			DEBUG_TRACE_STACK;
		}
#endif
		asm volatile("pause");
	}
}

bool mutexTryAcquire(g_mutex* mutex, uint32_t owner)
{
	MUTEX_GUARD;

	bool set = false;

	bool intr = interruptsAreEnabled();
	if(intr)
		interruptsDisable();

	G_SPINLOCK_ACQUIRE(mutex->lock);

	if(mutex->depth == 0 || mutex->owner == owner)
	{
		if(systemIsReady() && mutex->depth == 0)
		{
			auto local = taskingGetLocal();
			if(local->lockCount == 0)
				local->lockSetIF = intr;
			local->lockCount++;
		}
		++mutex->depth;
		mutex->owner = owner;
		set = true;
	}

	G_SPINLOCK_RELEASE(mutex->lock);

	return set;
}

void mutexRelease(g_mutex* mutex)
{
	MUTEX_GUARD;

	bool intr = false;

	G_SPINLOCK_ACQUIRE(mutex->lock);

	if(mutex->depth > 0)
	{
		--mutex->depth;
		if(mutex->depth == 0)
		{
			if(systemIsReady())
			{
				auto local = taskingGetLocal();
				local->lockCount--;
				if(local->lockCount == 0)
					intr = local->lockSetIF;
			}

			mutex->owner = -1;
		}
	}

	G_SPINLOCK_RELEASE(mutex->lock);

	if(intr)
		interruptsEnable();
}
