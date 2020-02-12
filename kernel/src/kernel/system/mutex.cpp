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
#include "kernel/system/interrupts/interrupts.hpp"

#include "shared/logger/logger.hpp"
#include "shared/video/console_video.hpp"

volatile int mutexInitializerLock = 0;
#define G_MUTEX_INITIALIZED 0xFEED

void mutexErrorUninitialized(g_mutex* mutex)
{
	loggerPrintlnUnlocked("%! %i: tried to use uninitialized mutex %h", "mutex", processorGetCurrentId(), mutex);
	for(;;);
}

void mutexInitialize(g_mutex* mutex)
{
	while(!__sync_bool_compare_and_swap(&mutexInitializerLock, 0, 1))
		asm("pause");

	if(mutex->initialized != G_MUTEX_INITIALIZED) {
		mutex->initialized = G_MUTEX_INITIALIZED;
		mutex->lock = 0;
		mutex->depth = 0;
		mutex->owner = -1;
	}

	mutexInitializerLock = 0;
}

void mutexAcquire(g_mutex* mutex)
{
	mutexAcquire(mutex, true);
}

void mutexAcquire(g_mutex* mutex, bool smp)
{
	if(mutex->initialized != G_MUTEX_INITIALIZED)
		mutexErrorUninitialized(mutex);

	while(!mutexTryAcquire(mutex, smp))
		asm("pause");
}

bool mutexTryAcquire(g_mutex* mutex)
{
	return mutexTryAcquire(mutex, true);
}

bool mutexTryAcquire(g_mutex* mutex, bool smp)
{
	if(mutex->initialized != G_MUTEX_INITIALIZED)
		mutexErrorUninitialized(mutex);

	// Disable interrupts
	bool enableInt = interruptsAreEnabled();
	interruptsDisable();

	// Lock editing
	while(!__sync_bool_compare_and_swap(&mutex->lock, 0, 1))
		asm("pause");

	// Update mutex
	bool success = false;
	if(mutex->depth == 0)
	{
		mutex->owner = processorGetCurrentId();
		mutex->depth = 1;
		if(smp) {
			if(taskingGetLocal()->locksHeld == 0)
				taskingGetLocal()->locksReenableInt = enableInt;
			taskingGetLocal()->locksHeld++;
		}
		success = true;

	} else if(mutex->owner == processorGetCurrentId())
	{
		mutex->depth++;
		success = true;
	}

	// Allow editing again
	mutex->lock = 0;

	return success;
}

void mutexRelease(g_mutex* mutex)
{
	mutexRelease(mutex, true);
}

void mutexRelease(g_mutex* mutex, bool smp)
{
	if(mutex->initialized != G_MUTEX_INITIALIZED)
		mutexErrorUninitialized(mutex);

	bool enableInt = false;

	// Lock editing
	while(!__sync_bool_compare_and_swap(&mutex->lock, 0, 1))
		asm("pause");

	// Update mutex
	if(mutex->depth > 0 && --mutex->depth == 0)
	{
		mutex->depth = 0;
		mutex->owner = -1;

		if(smp) {
			taskingGetLocal()->locksHeld--;
			if(taskingGetLocal()->locksHeld == 0)
				enableInt = taskingGetLocal()->locksReenableInt;
		}
	}

	// Allow editing again
	mutex->lock = 0;

	// Enable interrupts
	if(enableInt) interruptsEnable();
}

