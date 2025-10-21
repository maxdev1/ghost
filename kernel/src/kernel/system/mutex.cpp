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
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/panic.hpp"

#define G_MUTEX_INITIALIZED 0xFEED
#define G_MUTEX_MAX_PAUSES 1024
#define G_MUTEX_NO_OWNER (-1)

g_spinlock mutexInitializerLock = 0;

bool _mutexTryAcquire(g_mutex* mutex, uint32_t owner, bool hadIF);
void _mutexInitialize(g_mutex* mutex, g_mutex_type type, const char* location);

void mutexErrorUninitialized(g_mutex* mutex)
{
	panic("%! %i: tried to use uninitialized mutex %h", "mutex", processorGetCurrentId(), mutex);
}

void mutexInitializeTask(g_mutex* mutex, const char* location)
{
	_mutexInitialize(mutex, G_MUTEX_TYPE_TASK, location);
}

void mutexInitializeGlobal(g_mutex* mutex, const char* location)
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
	if(mutex->initialized != G_MUTEX_INITIALIZED)
		mutexErrorUninitialized(mutex);

	// Choose targeted owner value
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
			panic("%! early acquire of task-mutex initialized at <%s>", "mutex", mutex->location);
	}

	// No interruption during acquiry, only explicit yield allowed
	bool hadIF = interruptsAreEnabled();
	interruptsDisable();

	int deadlock = 0;
	uint32_t pauses = 1;
	while(!_mutexTryAcquire(mutex, owner, hadIF))
	{
		// As long as any global mutex is locked, we may never yield
		if(mutex->type == G_MUTEX_TYPE_GLOBAL || taskingGetLocal()->locking.globalLockCount > 0)
		{
			for(uint32_t i = 0; i < pauses; i++)
				asm volatile("pause");
			pauses *= 2;
			if(pauses > G_MUTEX_MAX_PAUSES)
				pauses = G_MUTEX_MAX_PAUSES;
		}
		else
		{
			taskingYield();
		}

		// Check for deadlocks
		++deadlock;
		if(deadlock % 100000 == 0)
			logDebug("%! long lock on processor %i initialized at %s, owner is: %i", "mutex", processorGetCurrentId(),
					 mutex->location, mutex->owner);
	}

	// Only for task mutexes (and if previously enabled) interrupts are enabled again
	if(mutex->type == G_MUTEX_TYPE_TASK && hadIF)
		interruptsEnable();
}

bool _mutexTryAcquire(g_mutex* mutex, uint32_t owner, bool hadIF)
{
	bool wasSet = false;

	G_SPINLOCK_ACQUIRE(mutex->lock);

	if(mutex->depth == 0 || mutex->owner == owner)
	{
		// On the first acquire of each global mutex, increase global lock count
		if(mutex->depth == 0 && mutex->type == G_MUTEX_TYPE_GLOBAL && systemIsReady())
		{
			auto local = taskingGetLocal();

			// Only store IF state if no other global mutex is locked yet
			if(local->locking.globalLockCount == 0)
				local->locking.globalLockSetIFAfterRelease = hadIF;

			local->locking.globalLockCount++;
		}

		// Increase reentrancy depth and update owner
		++mutex->depth;
		mutex->owner = owner;
		wasSet = true;
	}

	G_SPINLOCK_RELEASE(mutex->lock);

	return wasSet;
}

void mutexRelease(g_mutex* mutex)
{
	if(mutex->initialized != G_MUTEX_INITIALIZED)
		mutexErrorUninitialized(mutex);

	// No interruption allowed during check
	bool setIF = false;
	interruptsDisable();

	G_SPINLOCK_ACQUIRE(mutex->lock);

	if(mutex->depth > 0)
	{
		--mutex->depth;

		if(mutex->depth == 0)
		{
			// On last release of each global mutex, decrease global lock count
			if(mutex->type == G_MUTEX_TYPE_GLOBAL && systemIsReady())
			{
				auto local = taskingGetLocal();
				local->locking.globalLockCount--;

				// On last release of the last locked global mutex, restore IF state
				if(local->locking.globalLockCount == 0)
					setIF = local->locking.globalLockSetIFAfterRelease;
			}

			// Remove owner
			mutex->owner = -1;
		}
	}

	G_SPINLOCK_RELEASE(mutex->lock);

	// Restore IF state according to rules above
	if(setIF)
		interruptsEnable();
}
