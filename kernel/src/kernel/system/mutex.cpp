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
#include "kernel/kernel.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/logger/logger.hpp"
#include "shared/video/console_video.hpp"

volatile int mutexInitializerLock = 0;
#define G_MUTEX_INITIALIZED 0xFEED

void mutexErrorUninitialized(g_mutex* mutex)
{
    kernelPanic("%! %i: tried to use uninitialized mutex %h", "mutex", processorGetCurrentId(), mutex);
}

void mutexInitialize(g_mutex* mutex)
{
    while(!__sync_bool_compare_and_swap(&mutexInitializerLock, 0, 1))
        asm("pause");

    if(mutex->initialized == G_MUTEX_INITIALIZED)
        kernelPanic("%! attempted to initialize mutex %x twice", "mutex", mutex);

    mutex->initialized = G_MUTEX_INITIALIZED;
    mutex->lock = 0;
    mutex->depth = 0;
    mutex->owner = -1;

    mutexInitializerLock = 0;
}

void mutexAcquire(g_mutex* mutex)
{
    if(mutex->initialized != G_MUTEX_INITIALIZED)
        mutexErrorUninitialized(mutex);

    while(!mutexTryAcquire(mutex))
    {
        if(interruptsAreEnabled())
            taskingYield();
        else
            asm("pause");
    }
}

bool mutexTryAcquire(g_mutex* mutex)
{
    if(mutex->initialized != G_MUTEX_INITIALIZED)
        mutexErrorUninitialized(mutex);

    while(!__sync_bool_compare_and_swap(&mutex->lock, 0, 1))
        asm("pause");

    int intr = interruptsAreEnabled();
    if(intr)
        interruptsDisable();

    bool set = false;

    g_task* task = taskingGetCurrentTask();
    uint32_t owner = task ? task->id : -processorGetCurrentId();
    if(mutex->depth == 0 || mutex->owner == owner)
    {
        mutex->owner = owner;
        mutex->depth++;
        set = true;
    }

    mutex->lock = 0;

    if(intr)
        interruptsEnable();

    return set;
}

void mutexRelease(g_mutex* mutex)
{
    if(mutex->initialized != G_MUTEX_INITIALIZED)
        mutexErrorUninitialized(mutex);

    while(!__sync_bool_compare_and_swap(&mutex->lock, 0, 1))
        asm("pause");

    int intr = interruptsAreEnabled();
    if(intr)
        interruptsDisable();

    if(mutex->depth > 0 && --mutex->depth == 0)
    {
        mutex->depth = 0;
        mutex->owner = -1;
    }

    mutex->lock = 0;

    if(intr)
        interruptsEnable();
}
