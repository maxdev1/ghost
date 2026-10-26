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


#include "kernel/tasking/scheduler/scheduler.hpp"
#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/processor/processor.hpp"

struct g_irq_registration
{
	g_task* task;
	bool waiting;
	bool pending;
};

static g_irq_registration registrations[256] = {};
static g_mutex registrationsLock;

void requestsInitialize()
{
	mutexInitializeGlobal(&registrationsLock, __func__);
	for(int i = 0; i < 256; i++)
	{
		registrations[i].task = nullptr;
		registrations[i].waiting = false;
		registrations[i].pending = false;
	}
}

void requestsSetHandlerTask(uint8_t irq, g_task* task)
{
	mutexAcquire(&registrationsLock);
	registrations[irq].task = task;
	registrations[irq].waiting = false;
	mutexRelease(&registrationsLock);
}

bool requestsWaitForIrq(uint8_t irq, g_task* task)
{
	mutexAcquire(&registrationsLock);

	auto& registration = registrations[irq];
	registration.task = task;

	bool shouldWait = !registration.pending;
	if(shouldWait)
	{
		taskingPrepareWait(task, "irq");
		registration.waiting = true;
	}
	else
	{
		registration.pending = false;
		registration.waiting = false;
	}

	mutexRelease(&registrationsLock);
	return shouldWait;
}

void requestsClearWait(uint8_t irq, g_task* task)
{
	mutexAcquire(&registrationsLock);

	auto& registration = registrations[irq];
	if(registration.task == task)
		registration.waiting = false;

	mutexRelease(&registrationsLock);
}

g_task* requestsGetHandlerTask(uint8_t irq)
{
	mutexAcquire(&registrationsLock);
	g_task* task = registrations[irq].task;
	mutexRelease(&registrationsLock);
	return task;
}

void requestsTaskRemoved(g_task* task)
{
	mutexAcquire(&registrationsLock);
	for(int i = 0; i < 256; i++)
	{
		if(registrations[i].task == task)
		{
			registrations[i].task = nullptr;
			registrations[i].waiting = false;
		}
	}
	mutexRelease(&registrationsLock);
}

void requestsHandle(g_task* currentTask, uint8_t irq)
{
	g_task* handlerTask = nullptr;
	bool wakeHandler = false;
	bool resumeHandler = false;

	mutexAcquire(&registrationsLock);
	auto& registration = registrations[irq];
	if(registration.task)
		handlerTask = taskingAcquireTask(registration.task->id);
	if(handlerTask && registration.waiting)
	{
		registration.waiting = false;
		wakeHandler = true;
	}
	else
	{
		registration.pending = true;
		// A driver may have yielded to its consumer before returning to await_irq.
		// Its saved state is still runnable, and should drain this IRQ promptly.
		resumeHandler = handlerTask && handlerTask != currentTask && handlerTask->assignment &&
		                handlerTask->assignment->processor == processorGetCurrentId();
	}
	mutexRelease(&registrationsLock);

	if(wakeHandler)
		taskingWake(handlerTask);
	else if(resumeHandler)
	{
		// Only a same-core task can reach here: local cleanup cannot destroy it
		// while this interrupt runs. Avoid registration->task lock inversion with
		// task destruction, and never wake a driver waiting for a mutex or I/O.
		mutexAcquire(&handlerTask->lock);
		resumeHandler = handlerTask->status == G_TASK_STATUS_RUNNING;
		mutexRelease(&handlerTask->lock);
	}

	if((!wakeHandler && !resumeHandler) || !handlerTask->assignment ||
	   handlerTask->assignment->processor != processorGetCurrentId())
	{
		taskingReleaseTask(handlerTask);
		return;
	}

	taskingSetCurrent(handlerTask);

	// Once the handler has finished, let the scheduler go back to interrupted task
	if(currentTask)
		schedulerPreferTask(currentTask);
	taskingReleaseTask(handlerTask);
}
