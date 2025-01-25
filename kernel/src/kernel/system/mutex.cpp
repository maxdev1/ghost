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
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/panic.hpp"
#include "shared/video/console_video.hpp"

g_spinlock mutexInitializerLock = 0;
#define G_MUTEX_INITIALIZED 0xFEED

#define MUTEX_GUARD                               \
	if(mutex->initialized != G_MUTEX_INITIALIZED) \
		mutexErrorUninitialized(mutex);

bool _mutexTryAcquire(g_mutex* mutex, uint32_t owner);
void _mutexInitialize(g_mutex* mutex, bool disablesInterrupts);

void mutexErrorUninitialized(g_mutex* mutex)
{
	panic("%! %i: tried to use uninitialized mutex %h", "mutex", processorGetCurrentId(), mutex);
}

void mutexInitialize(g_mutex* mutex)
{
	_mutexInitialize(mutex, false);
}

void mutexInitializeCritical(g_mutex* mutex)
{
	_mutexInitialize(mutex, true);
}

void _mutexInitialize(g_mutex* mutex, bool disablesInterrupts)
{
	G_SPINLOCK_ACQUIRE(mutexInitializerLock);

	mutex->initialized = G_MUTEX_INITIALIZED;
	mutex->lock = 0;
	mutex->depth = 0;
	mutex->owner = -1;
	mutex->disablesInterrupts = disablesInterrupts;

	G_SPINLOCK_RELEASE(mutexInitializerLock);
}

void mutexAcquire(g_mutex* mutex)
{
	MUTEX_GUARD;

	uint32_t owner = processorGetCurrentId();

	while(!_mutexTryAcquire(mutex, owner))
	{
		if(mutex->disablesInterrupts)
			asm("pause");
		else
			taskingYield();
	}
}

bool _mutexTryAcquire(g_mutex* mutex, uint32_t owner)
{
	MUTEX_GUARD;

	bool wasSet = false;
	bool hadIF = interruptsAreEnabled();
	interruptsDisable();

	G_SPINLOCK_ACQUIRE(mutex->lock);

	if(mutex->depth == 0 || mutex->owner == owner)
	{
		if(systemIsReady() && mutex->depth == 0)
		{
			auto local = taskingGetLocal();
			if(local->lockCount == 0)
				local->lockSetIF = hadIF;
			local->lockCount++;
		}
		++mutex->depth;
		mutex->owner = owner;
		wasSet = true;
	}

	G_SPINLOCK_RELEASE(mutex->lock);

	if(!mutex->disablesInterrupts && hadIF)
		interruptsEnable();

	return wasSet;
}

void mutexRelease(g_mutex* mutex)
{
	MUTEX_GUARD;

	bool hadIF = false;
	interruptsDisable();

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
					hadIF = local->lockSetIF;
			}

			mutex->owner = -1;
		}
	}

	G_SPINLOCK_RELEASE(mutex->lock);

	if(hadIF)
		interruptsEnable();
}
