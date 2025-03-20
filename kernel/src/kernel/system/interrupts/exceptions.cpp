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
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/processor/virtual_8086_monitor.hpp"
#include "kernel/tasking/elf/elf_loader.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "shared/logger/logger.hpp"
#include "shared/panic.hpp"

#define DEBUG_PRINT_STACK_TRACE 1

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
		"reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception",
		"reserved exception",
		"reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception",
		"reserved exception", "reserved exception" // reserved exceptions
};

g_address exceptionsGetCR2()
{
	g_address addr;
	asm volatile("mov %%cr2, %0"
		: "=r"(addr));
	return addr;
}

bool exceptionsHandleDivideError(g_task* task)
{
	uint8_t* faultyInstruction = (uint8_t*) task->state->rip;

	uint8_t opcode = faultyInstruction[0];
	int skip = 1;

	if(
		opcode == 0xF6 // DIV
		|| opcode == 0xF7 // IDIV
	)
	{
		skip = 2;
		uint8_t modrm = faultyInstruction[1];
		uint8_t mod = (modrm >> 6) & 0x3;
		if(mod == 1)
			skip += 1;
		else if(mod == 2)
			skip += 4;
	}

	task->state->rip += skip;

	logInfo("%! divide error in task %i at %x, skipping to EIP: %x", "exception", task->id, faultyInstruction,
	        task->state->rip);
	return true;
}

/**
 * Dumps the current CPU state to the log file
 */
void exceptionsDumpTask(g_task* task)
{
	auto state = task->state;
	g_process* process = task->process;
	logInfo("%! %s in task %i (process %i)", "exception", EXCEPTION_NAMES[state->intr], task->id, process->main->id);

	if(state->intr == 0x0E)
	{
		// Page fault
		logInfo("%#    accessed address: %h", exceptionsGetCR2());
	}
	logInfo("%#    rip: %h   eflags: %h", state->rip, state->eflags);
	logInfo("%#    rax: %h      rbx: %h", state->rax, state->rbx);
	logInfo("%#    rcx: %h      rdx: %h", state->rcx, state->rdx);
	logInfo("%#    rsp: %h      rbp: %h", state->rsp, state->rbp);
	logInfo("%#   intr: %h    error: %h", state->intr, state->error);
	logInfo("%#   task stack: %h - %h", task->stack.start, task->stack.end);
	logInfo("%#   intr stack: %h - %h", task->interruptStack.start, task->interruptStack.end);

	auto iter = hashmapIteratorStart(task->process->object->loadedObjects);
	while(hashmapIteratorHasNext(&iter))
	{
		auto object = hashmapIteratorNext(&iter)->value;

		logInfo("%# obj %x-%x: %s", object->startAddress, object->endAddress, object->name);

		if(state->rip >= object->startAddress && state->rip < object->endAddress)
		{
			if(object == task->process->object)
			{
				logInfo("%# caused in executable object");
			}
			else
			{
				logInfo("%# caused in object '%s' at offset %x", object->name, state->rip - object->baseAddress);
			}
			break;
		}
	}
	hashmapIteratorEnd(&iter);

#if DEBUG_PRINT_STACK_TRACE
	g_address* rbp = reinterpret_cast<g_address*>(state->rbp);
	logInfo("%# stack trace:");
	for(int frame = 0; frame < 8; ++frame)
	{
		g_address rip = rbp[1];
		if(rip < 0x1000)
		{
			break;
		}
		rbp = reinterpret_cast<g_address*>(rbp[0]);
		logInfo("%#  %h", rip);
	}
#endif
}

bool exceptionsHandlePageFault(g_task* task)
{
	g_virtual_address accessed = exceptionsGetCR2();

	if(taskingMemoryHandleStackOverflow(task, accessed))
		return true;

	if(memoryOnDemandHandlePageFault(task, accessed))
		return true;

	g_physical_address physPage = pagingVirtualToPhysical(G_PAGE_ALIGN_DOWN(accessed));
	logInfo("%! task %i (core %i) EIP: %x (accessed %h, mapped page %h)", "pagefault", task->id,
	        processorGetCurrentId(), task->state->rip, accessed, physPage);

	exceptionsDumpTask(task);

	if(task->type == G_TASK_TYPE_VITAL)
	{
		return false;
	}
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		task->status = G_TASK_STATUS_DEAD;
	}
	else
	{
		// TODO Somehow give the user task a chance to do something
		task->status = G_TASK_STATUS_DEAD;
	}
	taskingSchedule();
	return true;
}

bool exceptionsHandleGeneralProtectionFault(g_task* task)
{
	if(task->type == G_TASK_TYPE_VM86)
	{

		g_virtual_monitor_handling_result result = vm86MonitorHandleGpf(task);

		if(result == VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL)
		{
			return true;
		}
		else if(result == VIRTUAL_MONITOR_HANDLING_RESULT_FINISHED)
		{
			task->status = G_TASK_STATUS_DEAD;
			taskingSchedule();
			return true;
		}
		else if(result == VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE)
		{
			logInfo("%! %i unable to handle gpf for vm86 task", "exception", processorGetCurrentId());
			task->status = G_TASK_STATUS_DEAD;
			taskingSchedule();
			return true;
		}
	}

	exceptionsDumpTask(task);

	logInfo("%! #%i task %i killed due to general protection fault at EIP %h", "exception", processorGetCurrentId(),
	        task->id, task->state->rip);
	task->status = G_TASK_STATUS_DEAD;
	taskingSchedule();
	return true;
}

bool exceptionsKillTask(g_task* task)
{
	logInfo("%! task %i killed due to exception %i (error %i) at EIP %h", "exception", task->id, task->state->intr,
	        task->state->error, task->state->rip);
	task->status = G_TASK_STATUS_DEAD;
	taskingSchedule();
	return true;
}

void exceptionsHandle(g_task* task)
{
	bool resolved = false;

	if(!task)
		panic("%! unresolved exception before initializing tasking system", "exceptions");

	switch(task->state->intr)
	{
		case 0x00:
		{
			// Divide error
			resolved = exceptionsHandleDivideError(task);
			break;
		}
		case 0x0E:
		{
			// Page fault
			resolved = exceptionsHandlePageFault(task);
			break;
		}
		case 0x0D:
		{
			// General protection fault
			resolved = exceptionsHandleGeneralProtectionFault(task);
			break;
		}
		case 0x06:
		{
			// Invalid operation code
			resolved = exceptionsKillTask(task);
			break;
		}
	}

	if(!resolved)
	{
		logInfo("%*%! task %i caused unresolved exception %i (error %i) at EIP: %h ESP: %h", 0x0C, "exception",
		        task->id, task->state->intr,
		        task->state->error, task->state->rip, task->state->rsp);
		for(;;)
		{
			asm("hlt");
		}
	}
}
