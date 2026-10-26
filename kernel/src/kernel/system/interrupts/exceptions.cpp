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
#include "kernel/memory/constants.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/processor/virtual_8086_monitor.hpp"
#include "kernel/tasking/elf/elf_loader.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/panic.hpp"

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
	return false;
}

g_elf_object* exceptionsFindResponsibleObject(g_task* task, g_address rip)
{
	g_elf_object* object = nullptr;
	if(!task || !task->process || !task->process->object)
		return nullptr;

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

static bool exceptionsReadUser(g_address address, void* output, size_t size)
{
	if(address > G_MEM_LOWER_HALF_END || size > G_MEM_LOWER_HALF_END - address + 1)
		return false;
	auto bytes = (uint8_t*) output;
	while(size)
	{
		const auto entry = pagingVirtualToPageEntry(address);
		if((entry & (G_PAGE_PRESENT | G_PAGE_USER_FLAG)) != (G_PAGE_PRESENT | G_PAGE_USER_FLAG) ||
		   (entry & (G_PAGE_UNMANAGED_FLAG | G_PAGE_LARGE_PAGE_FLAG)))
			return false;
		const auto offset = address & G_PAGE_ALIGN_MASK;
		const auto count = size < G_PAGE_SIZE - offset ? size : G_PAGE_SIZE - offset;
		memoryCopy(bytes, (void*) G_MEM_PHYS_TO_VIRT((entry & G_PAGE_PHYSICAL_MASK) + offset), count);
		address += count;
		bytes += count;
		size -= count;
	}
	return true;
}

static bool exceptionsFindFunctionSymbol(g_elf_object* object, g_address rip, Elf64_Sym* best)
{
	bool found = false;
	const auto count = object->dynamicSymbolTableSize < 65536 ? object->dynamicSymbolTableSize : 65536;
	for(uint64_t i = 0; i < count; ++i)
	{
		Elf64_Sym symbol;
		const auto table = (g_address) object->dynamicSymbolTable;
		if(table > G_MEM_LOWER_HALF_END || i > (G_MEM_LOWER_HALF_END - table) / sizeof(symbol) ||
		   !exceptionsReadUser(table + i * sizeof(symbol), &symbol, sizeof(symbol)))
			break;
		if(ELF64_ST_TYPE(symbol.st_info) == STT_FUNC && symbol.st_value <= rip - object->baseAddress &&
		   (!found || symbol.st_value > best->st_value))
		{
			*best = symbol;
			found = true;
		}
	}
	return found;
}

void exceptionsPrintCallAtRip(g_task* task, g_address rip)
{
	auto object = exceptionsFindResponsibleObject(task, rip);
	Elf64_Sym function;
	char name[128];
	bool named = object && exceptionsFindFunctionSymbol(object, rip, &function) &&
	             function.st_name < object->dynamicStringTableSize;
	if(named)
	{
		size_t i = 0;
		for(; i + 1 < sizeof(name); ++i)
		{
			const uint64_t offset = (uint64_t) function.st_name + i;
			const auto table = (g_address) object->dynamicStringTable;
			if(offset >= object->dynamicStringTableSize || table > G_MEM_LOWER_HALF_END ||
			   offset > G_MEM_LOWER_HALF_END - table || !exceptionsReadUser(table + offset, &name[i], 1))
			{
				named = false;
				break;
			}
			if(!name[i])
				break;
		}
		name[i] = 0;
	}

	if(named)
	{
		logInfo("%#   %h (%s <%s> %h)", rip, object->name, name, rip - object->baseAddress);
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
	if(!task || (state->cs & 3) != 3)
		return;
	mutexAcquire(&task->process->lock);
	logInfo("%# stack trace:");
	exceptionsPrintCallAtRip(task, state->rip);

	auto rbp = state->rbp;
	for(int frame = 0; frame < 25; ++frame)
	{
		g_address words[2];
		if((rbp & (sizeof(g_address) - 1)) || rbp < task->stack.start + G_PAGE_SIZE ||
		   rbp > task->stack.end || sizeof(words) > task->stack.end - rbp ||
		   !exceptionsReadUser(rbp, words, sizeof(words)))
			break;
		g_address rip = words[1];
		if(rip < 0x1000)
		{
			break;
		}
		exceptionsPrintCallAtRip(task, rip);
		if(words[0] <= rbp)
			break;
		rbp = words[0];
	}
	mutexRelease(&task->process->lock);
}

/**
 * Dumps the current CPU state to the log file
 */
void exceptionsDumpState(g_task* task, volatile g_processor_state* state)
{
	g_process* process = task ? task->process : nullptr;
	logInfo("%! (task %i, process %i) %s", "exception", task ? task->id : G_TID_NONE,
	        process ? process->id : G_TID_NONE, state->intr < 32 ? EXCEPTION_NAMES[state->intr] : "unknown");

	if(state->intr == 0x0E)
	{
		// Page fault
		logInfo("%#    accessed address: %h", exceptionsGetCR2());
	}
	logInfo("%#   INTR: %h    ERROR: %h", state->intr, state->error);
	logInfo("%#    RIP: %h   RFLAGS: %h", state->rip, state->rflags);
	logInfo("%#    CS:  %h       SS: %h", state->cs, state->ss);
	logInfo("%#    RSP: %h      RBP: %h", state->rsp, state->rbp);
	logInfo("%#    RAX: %h  RBX: %h  %RCX: %h  RDX: %h", state->rax, state->rbx, state->rcx, state->rdx);
	if(task)
	{
		logInfo("%#   task stack: %h - %h", task->stack.start, task->stack.end);
		logInfo("%#   intr stack: %h - %h", task->interruptStack.start, task->interruptStack.end);
	}
	// A kernel fault may have interrupted a lock holder. Only immutable register
	// diagnostics are safe there; user faults have a clean syscall/lock boundary.
	if(task && (state->cs & 3) == 3 && task->process->object)
	{
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

static bool exceptionsTerminateUserTask(g_task* task, volatile g_processor_state* state)
{
	if(!task || (state->cs & 3) != 3 || task->type == G_TASK_TYPE_VITAL)
		return false;
	taskingTerminateProcess(task->process);
	mutexAcquire(&task->lock);
	task->status = G_TASK_STATUS_DEAD;
	mutexRelease(&task->lock);
	waitQueueWake(&task->waitersJoin);
	taskingSchedule();
	return true;
}

bool exceptionsHandlePageFault(g_task* task, volatile g_processor_state* state)
{
	g_virtual_address accessed = exceptionsGetCR2();
	g_physical_address pageEntryValue = pagingVirtualToPageEntry(G_PAGE_ALIGN_DOWN(accessed));

	if(task)
	{
		// Demand allocation can resolve a missing translation, never a protection
		// violation or a reserved-bit page-table fault.
		const bool missingPage = (state->error & (1 | 8)) == 0;
		if(missingPage && taskingMemoryHandleStackOverflow(task, accessed))
			return true;

		if(missingPage && memoryOnDemandHandlePageFault(task, accessed))
			return true;

		logInfo("%! (task %i, core %i) RIP: %x (accessed %h, mapping value: %h)", "pagefault", task->id,
		        processorGetCurrentId(), state->rip, accessed, pageEntryValue);

		exceptionsDumpState(task, state);

		return exceptionsTerminateUserTask(task, state);
	}

	logInfo("%! RIP: %x (accessed %h, mapping value: %h)", "pagefault", processorGetCurrentId(), state->rip, accessed,
	        pageEntryValue);
	return false;
}

bool exceptionsHandleGeneralProtectionFault(g_task* task, volatile g_processor_state* state)
{
	exceptionsDumpState(task, state);

	return exceptionsTerminateUserTask(task, state);
}

bool exceptionsKillTask(g_task* task)
{
	if(!task)
		return false;
	exceptionsDumpState(task, task->state);
	return exceptionsTerminateUserTask(task, task->state);
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
		if(task && (state->cs & 3) == 3 && state->intr != 2 && state->intr != 8 && state->intr != 18)
		{
			exceptionsDumpState(task, state);
			if(exceptionsTerminateUserTask(task, state))
				return;
		}
		panic("%! kernel/hardware exception %i (error %h) at RIP %h RSP %h", "exception",
		      state->intr, state->error, state->rip, state->rsp);
	}
}
