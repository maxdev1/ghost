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

void taskingStateResetVm86(g_task* task, g_vm86_registers in, uint32_t intr)
{
	g_processor_state_vm86* state = (g_processor_state_vm86*) (task->interruptStack.end - sizeof(g_processor_state_vm86));
	task->state = (g_processor_state*) state;

	memorySetBytes(state, 0, sizeof(g_processor_state_vm86));
	state->defaultFrame.eax = in.ax;
	state->defaultFrame.ebx = in.bx;
	state->defaultFrame.ecx = in.cx;
	state->defaultFrame.edx = in.dx;
	state->defaultFrame.ebp = 0;
	state->defaultFrame.esi = in.si;
	state->defaultFrame.edi = in.di;

	state->defaultFrame.eip = G_FP_OFF(ivt->entry[intr]);
	state->defaultFrame.cs = G_FP_SEG(ivt->entry[intr]);
	state->defaultFrame.eflags = 0x20202;
	state->defaultFrame.ss = ((G_PAGE_ALIGN_DOWN(task->stack.start) + G_PAGE_SIZE) >> 4);

	state->gs = 0x00;
	state->fs = 0x00;
	state->es = in.es;
	state->ds = in.ds;
}

void taskingStateReset(g_task* task, g_address eip, g_security_level entryLevel)
{
	g_processor_state* state;
	if(entryLevel == G_SECURITY_LEVEL_KERNEL && task->securityLevel > G_SECURITY_LEVEL_KERNEL)
		state = (g_processor_state*) (task->interruptStack.end - sizeof(g_processor_state));
	else
		state = (g_processor_state*) (task->stack.end - sizeof(g_processor_state));

	task->state = state;

	memorySetBytes((void*) task->state, 0, sizeof(g_processor_state));
	state->eflags = 0x200;
	state->esp = (g_virtual_address) task->state;

	if(entryLevel == G_SECURITY_LEVEL_KERNEL)
	{
		state->cs = G_GDT_DESCRIPTOR_KERNEL_CODE | G_SEGMENT_SELECTOR_RING0;
		state->ss = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->ds = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->es = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->fs = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
	}
	else
	{
		state->cs = G_GDT_DESCRIPTOR_USER_CODE | G_SEGMENT_SELECTOR_RING3;
		state->ss = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->ds = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->es = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->fs = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->gs = G_GDT_DESCRIPTOR_USERTHREADLOCAL;
	}

	if(entryLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		state->eflags |= 0x3000; // IOPL 3
	}

	state->eip = eip;
}