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

#include "kernel/system/interrupts/exceptions.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/system/processor/virtual_8086_monitor.hpp"

uint32_t exceptionsGetCR2()
{
	uint32_t addr;
	asm volatile("mov %%cr2, %0" : "=r"(addr));
	return addr;
}

bool exceptionsHandleDivideError(g_task* task)
{
	// Let process run, but skip the faulty instruction
	task->state->eip++;
	logInfo("%! thread %i had a divide error", "exception", task->id);
	return true;
}

bool exceptionsHandlePageFault(g_task* task)
{
	g_virtual_address accessedVirtual = G_PAGE_ALIGN_DOWN(exceptionsGetCR2());
	g_physical_address accessedPhysical = pagingVirtualToPhysical(accessedVirtual);

	logInfo("%! task %i (core %i) raised SIGSEGV @%x (virt %h, phys %h)", "pagefault", task->id, processorGetCurrentId(), task->state->eip, accessedVirtual, accessedPhysical);

	if(task->type == G_THREAD_TYPE_VITAL)
	{
		return false;
	}
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		task->status = G_THREAD_STATUS_DEAD;
	} else
	{
		taskingRaiseSignal(task, SIGSEGV);
	}
	taskingSchedule();
	return true;
}

bool exceptionsHandleGeneralProtectionFault(g_task* task)
{
	if (task->type == G_THREAD_TYPE_VM86) {

		g_virtual_monitor_handling_result result = vm86MonitorHandleGpf(task);

		if (result == VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL) {
			return true;

		} else if (result == VIRTUAL_MONITOR_HANDLING_RESULT_FINISHED) {
			task->status = G_THREAD_STATUS_DEAD;
			taskingSchedule();
			return true;

		} else if (result == VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE) {
			logInfo("%! %i unable to handle gpf for vm86 task", "exception", processorGetCurrentId());
			task->status = G_THREAD_STATUS_DEAD;
			taskingSchedule();
			return true;
		}
	}

	logInfo("%! #%i process %i killed due to general protection fault", "exception", processorGetCurrentId(), task->id);
	task->status = G_THREAD_STATUS_DEAD;
	taskingSchedule();
	return true;
}

bool exceptionsKillTask(g_task* task)
{
	logInfo("%! task %i killed due to exception %i (error %i) at EIP %h", "exception", task->id, task->state->intr,
		task->state->error, task->state->eip);
	task->status = G_THREAD_STATUS_DEAD;
	taskingSchedule();
	return true;
}

void exceptionsHandle(g_task* task)
{
	bool resolved = false;

	switch (task->state->intr) {
	case 0x00: { // Divide error
		resolved = exceptionsHandleDivideError(task);
		break;
	}
	case 0x0E: { // Page fault
		resolved = exceptionsHandlePageFault(task);
		break;
	}
	case 0x0D: { // General protection fault
		resolved = exceptionsHandleGeneralProtectionFault(task);
		break;
	}
	case 0x06: { // Invalid operation code
		resolved = exceptionsKillTask(task);
		break;
	}
	}

	if(!resolved)
	{
		logInfo("%*%! task %i caused unresolved exception %i (error %i) at EIP: %h ESP: %h", 0x0C, "exception", task->id, task->state->intr,
				task->state->error, task->state->eip, task->state->esp);
		for(;;)
		{
			asm("hlt");
		}
	}
}

