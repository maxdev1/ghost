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
#include "kernel/system/interrupts/interrupts.hpp"
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

	if(intr == 0x81)
	{
		// Special handling for when a kernel thread yields using <taskingKernelThreadYield>
		taskingSchedule();
	}
	else if(intr == 0x80)
	{
		syscallHandle(task);
	}
	else
	{
		const uint32_t irq = intr - 0x20;

		if(irq == 0) // Timer triggers scheduling
		{
			taskingGetLocal()->time += APIC_MILLISECONDS_PER_TICK;
			schedulerNewTimeSlot();
			taskingSchedule();
		}
		else if(irq < 256 && handlers[irq]) // User-space irq handling
		{
			requestsCallUserspaceHandler(irq);
		}
		else
		{
			logInfo("%! unhandled irq %i", "requests", irq);
		}
	}
}

void requestsCallUserspaceHandler(uint8_t irq)
{
	g_irq_handler* handler = handlers[irq];
	g_task* handlerTask = taskingGetById(handler->task);

	if(!handlerTask)
	{
		logInfo("%! no handler task for irq #%i", "irq", irq);
		return;
	}

	if(handlerTask->interruptedState)
	{
		logDebug("%! handling task %i interrupted while getting irq #%i", "irq", handlerTask->id, irq);
		return;
	}

	/**
	 * TODO: It might be necessary to pin driver threads to CPU #0. Think this through.
	 */
    taskingInterruptTask(handlerTask, handler->entryAddress, handler->returnAddress, 2, irq, handler->handlerAddress);
    taskingSetCurrentTask(handlerTask);
}

void requestsRegisterHandler(uint8_t irq, g_tid handlerTask, g_virtual_address handlerAddress, g_virtual_address entryAddress, g_virtual_address returnAddress)
{
	if(handlers[irq])
	{
		logInfo("%! tried to register handler for irq %i which is already taken", "irq", irq);
		return;
	}

	g_irq_handler* handler = (g_irq_handler*) heapAllocate(sizeof(g_irq_handler));
	handler->task = handlerTask;
	handler->returnAddress = returnAddress;
	handler->entryAddress = entryAddress;
	handler->handlerAddress = handlerAddress;
	handlers[irq] = handler;
}
