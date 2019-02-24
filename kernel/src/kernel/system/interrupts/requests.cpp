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
#include "shared/logger/logger.hpp"
#include "kernel/tasking/tasking.hpp"

g_processor_state* requestsHandle(g_processor_state* statePtr)
{
	g_tasking_local* local = taskingGetLocal();

	// Save register state
	if(local->current)
		local->current->state = *statePtr;
	else
		taskingSchedule();

	// Handle IRQ
	const uint32_t irq = statePtr->intr - 0x20;
	if(irq == 0)
	{
		taskingSchedule();
	} else
	{
		// ...
	}

	// Restore state from current task
	g_virtual_address tssEsp0 = local->kernelStack;
	gdtSetTssEsp0(tssEsp0);
	g_processor_state* state = (g_processor_state*) (tssEsp0 + G_PAGE_SIZE - sizeof(g_processor_state));
	*state = local->current->state;
	return state;
}
