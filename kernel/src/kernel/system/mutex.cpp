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
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/panic.hpp"
#include "shared/logger/logger.hpp"

g_spinlock mutexInitializerLock = 0;
#define G_MUTEX_INITIALIZED 0xFEED

#define MUTEX_GUARD                               \
	if(mutex->initialized != G_MUTEX_INITIALIZED) \
		mutexErrorUninitialized(mutex);

bool _mutexTryAcquire(g_mutex* mutex, uint32_t owner);
void _mutexInitialize(g_mutex* mutex, g_mutex_type type, const char* location);

void mutexErrorUninitialized(g_mutex* mutex)
{
	panic("%! %i: tried to use uninitialized mutex %h", "mutex", processorGetCurrentId(), mutex);
}

void mutexInitialize(g_mutex* mutex, const char* location)
{
	_mutexInitialize(mutex, G_MUTEX_TYPE_TASK, location);
}

void mutexInitializeNonInterruptible(g_mutex* mutex, const char* location)
{
	_mutexInitialize(mutex, G_MUTEX_TYPE_GLOBAL, location);
}

void _mutexInitialize(g_mutex* mutex, g_mutex_type type, const char* location)
{
	G_SPINLOCK_ACQUIRE(mutexInitializerLock);

	mutex->initialized = G_MUTEX_INITIALIZED;
	mutex->lock = 0;
	mutex->depth = 0;
	mutex->owner = -1;
	mutex->type = type;
	mutex->location = location;

	G_SPINLOCK_RELEASE(mutexInitializerLock);
}

void mutexAcquire(g_mutex* mutex)
{
	MUTEX_GUARD;

	uint32_t owner;
	if(mutex->type == G_MUTEX_TYPE_GLOBAL || !systemIsReady())
	{
		owner = processorGetCurrentId();
	}
	else
	{
		auto currentTask = taskingGetCurrentTask();
		if(currentTask)
			owner = currentTask->id;
		else
			panic("%! early acquire of mutex initialized at %s", "mutex", mutex->location);
	}

	int deadlock = 0;

	while(!_mutexTryAcquire(mutex, owner))
	{
		++deadlock;
		if(deadlock % 100000 == 0)
			logDebug("%! long lock initialized at %s", "mutex", mutex->location);

		if(mutex->type == G_MUTEX_TYPE_GLOBAL)
			asm volatile("pause");
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

	if(mutex->type == G_MUTEX_TYPE_TASK && hadIF)
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
