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
#include "kernel/utils/string.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/processor/virtual_8086_monitor.hpp"
#include "kernel/tasking/elf/elf_loader.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "kernel/logger/logger.hpp"

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
	if(!task)
		return false;

	auto faultyInstruction = (uint8_t*) task->state->rip;
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

	logInfo("%! divide error in task %i at %x, skipping to RIP: %x", "exception", task->id, faultyInstruction,
	        task->state->rip);
	return true;
}

g_elf_object* exceptionsFindResponsibleObject(g_task* task, g_address rip)
{
	g_elf_object* object = nullptr;

	auto iter = hashmapIteratorStart(task->process->object->loadedObjects);
	while(hashmapIteratorHasNext(&iter))
	{
		auto nextObject = hashmapIteratorNext(&iter)->value;
		if(rip >= nextObject->startAddress && rip < nextObject->endAddress)
		{
			object = nextObject;
			break;
		}
	}
	hashmapIteratorEnd(&iter);

	return object;
}

Elf64_Sym* exceptionsFindFunctionSymbol(g_elf_object* object, g_address rip)
{
	if(object->root)
	{
		// TODO: Symbol lookup only works well for shared libs so far, for some reason
		return nullptr;
	}

	Elf64_Sym* bestMatch = nullptr;
	g_address localRip = rip - object->baseAddress;

	for(uint64_t symbolIndex = 0; symbolIndex < object->dynamicSymbolTableSize; symbolIndex++)
	{
		auto& symbol = object->dynamicSymbolTable[symbolIndex];

		if(ELF64_ST_TYPE(symbol.st_info) == STT_FUNC)
		{
			if(localRip >= symbol.st_value &&
			   localRip < symbol.st_value + symbol.st_size)
			{
				return &symbol;
			}

			if(localRip >= symbol.st_value &&
			   (!bestMatch || symbol.st_value > bestMatch->st_value))
			{
				bestMatch = &symbol;
			}
		}
	}

	return bestMatch;
}

void exceptionsPrintCallAtRip(g_task* task, g_address rip)
{
	auto object = exceptionsFindResponsibleObject(task, rip);
	auto function = object ? exceptionsFindFunctionSymbol(object, rip) : nullptr;

	if(object && function)
	{
		auto functionName = function->st_name ? &object->dynamicStringTable[function->st_name] : "?";
		logInfo("%#   %h (%s <%s> %h)", rip, object->name, functionName, rip - object->baseAddress);
	}
	else if(object)
	{
		logInfo("%#   %h (%s %h)", rip, object->name, rip - object->baseAddress);
	}
	else
	{
		logInfo("%#   %h", rip);
	}
}

void exceptionsPrintStackTrace(g_task* task, volatile g_processor_state* state)
{
	logInfo("%# stack trace:");
	exceptionsPrintCallAtRip(task, state->rip);

	auto rbp = reinterpret_cast<g_address*>(state->rbp);
	for(int frame = 0; frame < 25; ++frame)
	{
		g_address rip = rbp[1];
		if(rip < 0x1000)
		{
			break;
		}
		rbp = reinterpret_cast<g_address*>(rbp[0]);

		exceptionsPrintCallAtRip(task, rip);
	}
}

/**
 * Dumps the current CPU state to the log file
 */
void exceptionsDumpState(g_task* task, volatile g_processor_state* state)
{
	g_process* process = task ? task->process : nullptr;
	logInfo("%! (task %i, process %i) %s", "exception", task ? task->id : G_TID_NONE,
	        process ? process->main->id : G_TID_NONE, EXCEPTION_NAMES[state->intr]);

	if(state->intr == 0x0E)
	{
		// Page fault
		logInfo("%#    accessed address: %h", exceptionsGetCR2());
	}
	logInfo("%#   INTR: %h    ERROR: %h", state->intr, state->error);
	logInfo("%#    RIP: %h   RFLAGS: %h", state->rip, state->rflags);
	logInfo("%#    CS:  %h       SS: %h", state->cs, state->ss);
	logInfo("%#    RSP: %h      RBP: %h", state->rbp, state->rbp);
	logInfo("%#    RAX: %h  RBX: %h  %RCX: %h  RDX: %h", state->rax, state->rbx, state->rcx, state->rdx);
	if(task)
	{
		logInfo("%#   task stack: %h - %h", task->stack.start, task->stack.end);
		logInfo("%#   intr stack: %h - %h", task->interruptStack.start, task->interruptStack.end);

		logInfo("%# loaded objects:");
		auto iter = hashmapIteratorStart(task->process->object->loadedObjects);
		while(hashmapIteratorHasNext(&iter))
		{
			auto object = hashmapIteratorNext(&iter)->value;

			logInfo("%#   %x-%x: %s", object->startAddress, object->endAddress, object->name);
		}
		hashmapIteratorEnd(&iter);
	}

	exceptionsPrintStackTrace(task, state);
}

bool exceptionsHandlePageFault(g_task* task, volatile g_processor_state* state)
{
	g_virtual_address accessed = exceptionsGetCR2();
	g_physical_address pageEntryValue = pagingVirtualToPageEntry(G_PAGE_ALIGN_DOWN(accessed));

	if(task)
	{
		if(taskingMemoryHandleStackOverflow(task, accessed))
			return true;

		if(memoryOnDemandHandlePageFault(task, accessed))
			return true;

		logInfo("%! (task %i, core %i) RIP: %x (accessed %h, mapping value: %h)", "pagefault", task->id,
		        processorGetCurrentId(), state->rip, accessed, pageEntryValue);

		exceptionsDumpState(task, state);

		if(task->type == G_TASK_TYPE_VITAL)
			return false;

		if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
			task->status = G_TASK_STATUS_DEAD;
		else
			// TODO Somehow give the user task a chance to do something
			task->status = G_TASK_STATUS_DEAD;

		taskingSchedule();
		return true;
	}

	logInfo("%! RIP: %x (accessed %h, mapping value: %h)", "pagefault", processorGetCurrentId(), state->rip, accessed,
	        pageEntryValue);
	return false;
}

bool exceptionsHandleGeneralProtectionFault(g_task* task, volatile g_processor_state* state)
{
	exceptionsDumpState(task, state);

	if(!task)
		return false;

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

void exceptionsHandle(g_task* task, volatile g_processor_state* state)
{
	bool resolved = false;

	switch(state->intr)
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
			resolved = exceptionsHandlePageFault(task, state);
			break;
		}
		case 0x0D:
		{
			// General protection fault
			resolved = exceptionsHandleGeneralProtectionFault(task, state);
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
		logInfo("%*%! (task %i) unresolved exception %i (error %i) at RIP: %h RSP: %h", 0x0C, "exception",
		        task ? task->id : G_TID_NONE, state->intr,
		        state->error, state->rip, state->rsp);
		for(;;)
		{
			asm("hlt");
		}
	}
}
