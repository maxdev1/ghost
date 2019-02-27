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

#include "shared/memory/memory.hpp"
#include "shared/logger/logger.hpp"

#include "kernel/calls/syscalls.hpp"

void requestsHandle(g_task* task)
{
	// Handle IRQ
	const uint32_t irq = task->state.intr - 0x20;
	if(irq == 0)
	{
		taskingSchedule();
	} else if(irq == 0x60)
	{
		logInfo("#%i: Task %i (Priv %i) issued syscall: %i", processorGetCurrentId(), task->id,task->securityLevel, G_SYSCALL_CODE(task->state));
	} else
	{
		logInfo("received unhandled irq %i", task->state.intr);
		for(;;)
			;
	}
}
