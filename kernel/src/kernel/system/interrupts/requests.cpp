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

static g_tid registrations[256] = {};
static g_mutex registrationsLock;

void requestsInitialize()
{
	mutexInitializeGlobal(&registrationsLock, __func__);
	for(int i = 0; i < 256; i++)
	{
		registrations[i] = G_TID_NONE;
	}
}

void requestsSetHandlerTask(uint8_t irq, g_tid task)
{
	mutexAcquire(&registrationsLock);
	registrations[irq] = task;
	mutexRelease(&registrationsLock);
}

g_tid requestsGetHandlerTask(uint8_t irq)
{
	mutexAcquire(&registrationsLock);
	g_tid task = registrations[irq];
	mutexRelease(&registrationsLock);
	return task;
}

void requestsHandle(g_task* currentTask, uint8_t irq)
{
	g_tid handlerTid = requestsGetHandlerTask(irq);
	if(handlerTid == G_TID_NONE)
		return;

	auto handlerTask = taskingGetById(handlerTid);
	if(!handlerTask)
		return;

	taskingWake(handlerTask);
	taskingSetCurrent(handlerTask);

	// Once the handler has finished, let the scheduler go back to interrupted task
	if(currentTask)
		schedulerPrefer(currentTask->id);
}
