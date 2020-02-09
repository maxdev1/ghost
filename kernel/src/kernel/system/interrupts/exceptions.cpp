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
#include "kernel/tasking/elf/elf_loader.hpp"
#include "kernel/memory/page_reference_tracker.hpp"

#define DEBUG_PRINT_STACK_TRACE 0

/**
 * Names of the exceptions
 */
static const char* EXCEPTION_NAMES[] = {
		"divide error", // 0x00
		"debug exception", // 0x01
		"non-maskable interrupt exception", // 0x02
		"breakpoint exception", // 0x03
		"detected overflow", // 0x04
		"out of bounds", // 0x05
		"invalid operation code", // 0x06
		"no co-processor", // 0x07
		"double fault", // 0x08
		"co-processor segment overrun", // 0x09
		"Bad TSS exception", // 0x0A
		"segment not present", // 0x0B
		"stack fault", // 0x0C
		"general protection fault", // 0x0D
		"page fault", // 0x0E
		"unknown interrupt", // 0x0F
		"co-processor fault", // 0x10
		"alignment check exception", // 0x11
		"machine check exception", // 0x12
		"reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception",
		"reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception" // reserved exceptions
		};

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

/**
 * Dumps the current CPU state to the log file
 */
void exceptionsDumpTask(g_task* task) {
	auto state = task->state;
	g_process* process = task->process;
	logInfo("%! %s in task %i (process %i)", "exception", EXCEPTION_NAMES[state->intr], task->id, process->main->id);

	if (state->intr == 0x0E) { // Page fault
		logInfo("%#    accessed address: %h", exceptionsGetCR2());
	}
	logInfo("%#    eip: %h   eflags: %h", state->eip, state->eflags);
	logInfo("%#    eax: %h      ebx: %h", state->eax, state->ebx);
	logInfo("%#    ecx: %h      edx: %h", state->ecx, state->edx);
	logInfo("%#    esp: %h      ebp: %h", state->esp, state->ebp);
	logInfo("%#   intr: %h    error: %h", state->intr, state->error);
	logInfo("%#   task stack: %h - %h", task->stack.start, task->stack.end);
	logInfo("%#   intr stack: %h - %h", task->interruptStack.start, task->interruptStack.end);

	g_elf_object* object = task->process->object->relocateOrderFirst;
	while(object)
	{
		if(state->eip >= object->baseAddress && state->eip < object->endAddress)
		{
			if(object == task->process->object)
			{
				logInfo("%# caused in executable object");
			} else
			{
				logInfo("%# caused in object '%s' at offset %x", object->name, state->eip - object->baseAddress);
			}
			break;
		}
		object = object->relocateOrderNext;
	}

	#if DEBUG_PRINT_STACK_TRACE
	g_address* ebp = reinterpret_cast<g_address*>(state->ebp);
	logInfo("%# stack trace:");
	for(int frame = 0; frame < 8; ++frame) {
		g_address eip = ebp[1];
		if(eip == 0) {
			break;
		}
		ebp = reinterpret_cast<g_address*>(ebp[0]);
		logInfo("%#  %h", eip);
	}
	#endif
}

bool exceptionsHandleStackOverflow(g_task* task, g_virtual_address accessedVirtPage)
{
	/* Within stack range? */
	if(accessedVirtPage < task->stack.start || accessedVirtPage > task->stack.end)
	{
		return false;
	}

	/* Stack guard page */
	if(accessedVirtPage < task->stack.start + G_PAGE_SIZE)
	{
		logInfo("%! task %i page-faulted due to overflowing into stack guard page", "pagefault", task->id);
		return false;

	}
	
	/* Extend the stack */
	uint32_t tableFlags, pageFlags;
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		tableFlags = DEFAULT_KERNEL_TABLE_FLAGS;
		pageFlags = DEFAULT_KERNEL_PAGE_FLAGS;
	} else
	{
		tableFlags = DEFAULT_USER_TABLE_FLAGS;
		pageFlags = DEFAULT_USER_PAGE_FLAGS;
	}

	g_physical_address extendedStackPage = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	pageReferenceTrackerIncrement(extendedStackPage);
	pagingMapPage(accessedVirtPage, extendedStackPage, tableFlags, pageFlags);
	return true;
}

bool exceptionsHandlePageFault(g_task* task)
{
	g_virtual_address accessed = exceptionsGetCR2();
	g_virtual_address virtPage = G_PAGE_ALIGN_DOWN(accessed);
	g_physical_address physPage = pagingVirtualToPhysical(virtPage);

	if(exceptionsHandleStackOverflow(task, virtPage))
		return true;

	logInfo("%! task %i (core %i) EIP: %x (accessed %h, mapped page %h)", "pagefault", task->id, processorGetCurrentId(), task->state->eip, accessed, physPage);

	exceptionsDumpTask(task);

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

