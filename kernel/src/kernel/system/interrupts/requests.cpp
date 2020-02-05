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

#include "ghost/memory.h"

#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/scheduler.hpp"
#include "kernel/memory/memory.hpp"

#include "shared/logger/logger.hpp"

#include "kernel/calls/syscall.hpp"
#include "kernel/kernel.hpp"

#include "shared/system/mutex.hpp"
#include "shared/system/io_port.hpp"

g_irq_handler* handlers[256] = { 0 };

void requestsHandle(g_task* task)
{
	const uint32_t intr = task->state->intr;

	/* Special handling for when a kernel thread yields using <taskingKernelThreadYield> */
	if(intr == 0x81)
	{
		taskingSchedule();

	/* System calls */
	} else if(intr == 0x80)
	{
		syscallHandle(task);

	} else
	{
		const uint32_t irq = intr - 0x20;

		/* Timer interrupt request triggers the scheduler */
		if(irq == 0)
		{
			taskingGetLocal()->time += APIC_MILLISECONDS_PER_TICK;
			schedulerNewTimeSlot();
			taskingSchedule();

		/* User-space interrupt handling */
		} else if(irq < 256 && handlers[irq])
		{
			requestsCallUserspaceHandler(task, irq);

		} else
		{
			logInfo("%! unhandled irq %i in task %i", "requests", irq, task->id);
		}
	}
}

void requestsCallUserspaceHandler(g_task* task, uint8_t irq)
{
	g_irq_handler* handler = handlers[irq];
	g_task* handlingTask = taskingGetById(handler->task);

	if(!handlingTask)
	{
		logInfo("%! no handler task for irq #%i", "irq", irq);
		return;
	}

	if(handlingTask->interruptionInfo)
	{
		logInfo("%! handling task %i interrupted while getting irq #%i", "irq", handlingTask->id, irq);
		return;
	}

	taskingInterruptTask(handlingTask, handler->handlerAddress, handler->returnAddress, 1, irq);
	taskingPleaseSchedule(handlingTask);
	taskingSchedule();
}

void requestsRegisterHandler(uint8_t irq, g_tid handlerTask, g_virtual_address handlerAddress, g_virtual_address returnAddress)
{
	if(handlers[irq])
	{
		logInfo("%! tried to register handler for irq %i which is already taken", "irq", irq);
		return;
	}

	g_irq_handler* handler = (g_irq_handler*) heapAllocate(sizeof(g_irq_handler));
	handler->task = handlerTask;
	handler->returnAddress = returnAddress;
	handler->handlerAddress = handlerAddress;
	handlers[irq] = handler;
}
