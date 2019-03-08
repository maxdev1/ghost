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

#include "shared/memory/memory.hpp"
#include "shared/logger/logger.hpp"

#include "kernel/calls/syscall.hpp"
#include "kernel/kernel.hpp"

#include "shared/system/mutex.hpp"
#include "shared/system/io_port.hpp"

void requestsHandle(g_task* task)
{
	const uint32_t irq = task->state->intr - 0x20;
	
	if(irq == 0) // Timer interrupt
	{
		taskingGetLocal()->time += APIC_MILLISECONDS_PER_TICK;
		schedulerNewTimeSlot();
		taskingSchedule();

	} else if( irq == 0x61) // Yield from kernel thread
	{
		taskingSchedule();

	} else if(irq == 0x60) // System calls
	{
		syscallHandle(task);

	} else
	{
		if(task->state->intr == 33) {
			logInfo("%! key pressed: %c", "irq", ioPortReadByte(0x60));
		} else {
			logInfo("%! unhandled irq %i in task %i", "requests", task->state->intr, task->id);
		}
	}
}
