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

#include "kernel/tasking/tasking_state.hpp"
#include "kernel/memory/gdt.hpp"
#include "kernel/system/interrupts/ivt.hpp"

void taskingStateReset(g_task* task, g_address rip, g_security_level entryLevel)
{
	g_processor_state* state;
	if(entryLevel == G_SECURITY_LEVEL_KERNEL && task->securityLevel > G_SECURITY_LEVEL_KERNEL)
		state = (g_processor_state*) (task->interruptStack.end - sizeof(g_processor_state));
	else
		state = (g_processor_state*) (task->stack.end - sizeof(g_processor_state));

	task->state = state;

	memorySetBytes((void*) task->state, 0, sizeof(g_processor_state));
	state->rflags = 0x20202;
	state->rsp = (g_virtual_address) task->state;

	if(entryLevel == G_SECURITY_LEVEL_KERNEL)
	{
		state->cs = G_GDT_DESCRIPTOR_KERNEL_CODE | G_SEGMENT_SELECTOR_RING0;
		state->ss = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->ds = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->es = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
	}
	else
	{
		state->cs = G_GDT_DESCRIPTOR_USER_CODE | G_SEGMENT_SELECTOR_RING3;
		state->ss = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->ds = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->es = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
	}

	if(entryLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		state->rflags |= 0x3000; // IOPL 3
	}

	state->rip = rip;
}
