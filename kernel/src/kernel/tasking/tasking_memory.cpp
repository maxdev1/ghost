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

#include "kernel/tasking/tasking_memory.hpp"

#include <shared/memory/constants.hpp>

#include "kernel/system/system.hpp"
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/system/processor/processor.hpp"
#include "shared/logger/logger.hpp"
#include "shared/panic.hpp"

bool taskingMemoryExtendHeap(g_task* task, int32_t amount, g_address* outAddress)
{
	g_process* process = task->process;
	mutexAcquire(&process->lock);
	g_physical_address returnDirectory = taskingMemoryTemporarySwitchTo(task->process->pageSpace);

	// Initialize the heap if necessary
	if(process->heap.brk == 0)
	{
		g_virtual_address heapStart = process->image.end;

		g_physical_address phys = memoryPhysicalAllocate();
		pagingMapPage(heapStart, phys, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT);

		process->heap.brk = heapStart;
		process->heap.start = heapStart;
		process->heap.pages = 1;
	}

	// Calculate new address
	g_virtual_address oldBrk = process->heap.brk;
	g_virtual_address newBrk = oldBrk + amount;

	// Heap expansion is limited
	// TODO limit heap expansion again?
	bool success = false;
	// if(newBrk >= G_USER_MAXIMUM_HEAP_BREAK)
	// {
	// 	logInfo("%! process %i went out of memory during sbrk", "syscall", process->main->id);
	// 	*outAddress = -1;
	// }
	// else
	// {
	// Expand if necessary
	g_virtual_address virt_above;
	while(newBrk > (virt_above = process->heap.start + process->heap.pages * G_PAGE_SIZE))
	{
		g_physical_address phys = memoryPhysicalAllocate();
		pagingMapPage(virt_above, phys, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT);
		++process->heap.pages;
	}

	// Shrink if possible
	g_virtual_address virtAligned;
	while(newBrk < (virtAligned = process->heap.start + process->heap.pages * G_PAGE_SIZE - G_PAGE_SIZE))
	{
		g_physical_address phys = pagingVirtualToPhysical(virtAligned);
		pagingUnmapPage(virtAligned);
		memoryPhysicalFree(phys);

		--process->heap.pages;
	}

	process->heap.brk = newBrk;
	*outAddress = oldBrk;
	success = true;
	// }

	taskingMemoryTemporarySwitchBack(returnDirectory);
	mutexRelease(&process->lock);
	return success;
}

void taskingMemoryInitialize(g_task* task)
{
	taskingMemoryInitializeStacks(task);
	taskingMemoryInitializeUtility(task);
	taskingMemoryInitializeTls(task);
}

void taskingMemoryInitializeUtility(g_task* task)
{
	if(processorHasFeature(g_cpuid_standard_edx_feature::SSE))
	{
		// TODO Allocator not capable of aligned allocation
		task->fpu.stateMem = (uint8_t*) heapAllocate(G_SSE_STATE_SIZE + G_SSE_STATE_ALIGNMENT);
		task->fpu.state = (uint8_t*) G_ALIGN_UP((g_address) task->fpu.stateMem, G_SSE_STATE_ALIGNMENT);

		if(task->process && task->process->main && task->process->main != task)
		{
			memoryCopy(task->fpu.state, task->process->main->fpu.state, G_SSE_STATE_SIZE);
			task->fpu.stored = true;
		}
		else
		{
			memoryCopy(task->fpu.state, processorGetInitialFpuState(), G_SSE_STATE_SIZE);
		}
	}
	else
	{
		task->fpu.stateMem = nullptr;
		task->fpu.state = nullptr;
	}
	task->fpu.stored = false;
}

void taskingMemoryInitializeStacks(g_task* task)
{
	// Interrupt stack for ring 3 & VM86 tasks
	if(task->securityLevel != G_SECURITY_LEVEL_KERNEL)
	{
		task->interruptStack = taskingMemoryCreateStack(memoryVirtualRangePool, G_PAGE_TABLE_KERNEL_DEFAULT,
		                                                G_PAGE_KERNEL_DEFAULT, G_TASKING_MEMORY_INTERRUPT_STACK_PAGES);
	}
	else
	{
		task->interruptStack.start = 0;
		task->interruptStack.end = 0;
	}

	// Create task stack
	if(task->type == G_TASK_TYPE_VM86)
	{
		size_t stackSize = G_PAGE_SIZE;
		task->stack.start = (g_address) lowerHeapAllocate(stackSize);
		task->stack.end = task->stack.start + stackSize;
	}
	else if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		task->stack = taskingMemoryCreateStack(memoryVirtualRangePool, G_PAGE_TABLE_KERNEL_DEFAULT,
		                                       G_PAGE_KERNEL_DEFAULT, G_TASKING_MEMORY_KERNEL_STACK_PAGES);
	}
	else
	{
		task->stack = taskingMemoryCreateStack(task->process->virtualRangePool, G_PAGE_TABLE_USER_DEFAULT,
		                                       G_PAGE_USER_DEFAULT, G_TASKING_MEMORY_USER_STACK_PAGES);
	}
}

g_stack taskingMemoryCreateStack(g_address_range_pool* addressRangePool, uint32_t tableFlags, uint32_t pageFlags,
                                 int pages)
{
	g_virtual_address stackVirt = addressRangePoolAllocate(addressRangePool, pages);

	// Only allocate and map the last page of the stack; when the process faults, lazy-allocate more physical space.
	// The first page of the allocated virtual range is used as a "guard page" and makes the process fault when accessed.
	g_physical_address pagePhys = memoryPhysicalAllocate();
	g_address stackEnd = stackVirt + pages * G_PAGE_SIZE;
	pagingMapPage(stackEnd - G_PAGE_SIZE, pagePhys, tableFlags, pageFlags);

	g_stack stack;
	stack.start = stackVirt;
	stack.end = stackEnd;
	return stack;
}

void taskingMemoryDestroy(g_task* task)
{
	taskingMemoryDestroyStacks(task);
	taskingMemoryDestroyUtility(task);
	taskingMemoryDestroyTls(task);
}

void taskingMemoryDestroyUtility(g_task* task)
{
	if(task->fpu.stateMem)
	{
		heapFree(task->fpu.stateMem);
		task->fpu.stateMem = nullptr;
	}
}

void taskingMemoryDestroyStacks(g_task* task)
{
	// Remove interrupt stack
	if(task->interruptStack.start)
	{
		for(g_virtual_address virt = task->interruptStack.start; virt < task->interruptStack.end; virt += G_PAGE_SIZE)
		{
			g_physical_address phys = pagingVirtualToPhysical(virt);
			if(phys > 0)
			{
				memoryPhysicalFree(phys);
				pagingUnmapPage(virt);
			}
		}
		addressRangePoolFree(memoryVirtualRangePool, task->interruptStack.start);
	}

	// Remove task stack
	if(task->type == G_TASK_TYPE_VM86)
	{
		lowerHeapFree((void*) task->stack.start);
	}
	else if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		taskingMemoryDestroyStack(memoryVirtualRangePool, task->stack);
	}
	else
	{
		taskingMemoryDestroyStack(task->process->virtualRangePool, task->stack);
	}
}

void taskingMemoryDestroyStack(g_address_range_pool* addressRangePool, g_stack& stack)
{
	for(g_virtual_address page = stack.start; page < stack.end; page += G_PAGE_SIZE)
	{
		g_physical_address pagePhys = pagingVirtualToPhysical(page);
		if(!pagePhys)
			continue;

		memoryPhysicalFree(pagePhys);
		pagingUnmapPage(page);
	}

	addressRangePoolFree(addressRangePool, stack.start);
}

g_physical_address taskingMemoryCreatePageSpace()
{
	auto currentPml4 = (g_address*) G_MEM_PHYS_TO_VIRT(pagingGetCurrentSpace());

	g_physical_address newPml4Phys = memoryPhysicalAllocate();
	auto newPml4 = (g_address*) G_MEM_PHYS_TO_VIRT(newPml4Phys);

	// Copy all higher-level mappings
	for(size_t i = 0; i < 512; i++)
	{
		if(i >= 256 && currentPml4[i])
			newPml4[i] = currentPml4[i];
		else
			newPml4[i] = 0;
	}

	return newPml4Phys;
}

void taskingMemoryDestroyPageSpace(g_physical_address directory)
{
	g_physical_address returnDirectory = taskingMemoryTemporarySwitchTo(directory);

	// Clear mappings and free physical space above 4 MiB
	// TODO
	logInfo("%! taskingMemoryDestroyPageSpace not implemented", "todo");
	// g_page_directory directoryCurrent = (g_page_directory) G_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	// for(uint32_t ti = 1; ti < 1024; ti++)
	// {
	// 	if(!(directoryCurrent[ti] & G_PAGE_USER_FLAG))
	// 		continue;
	//
	// 	g_page_table tableMapped = ((g_page_table) G_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);
	// 	for(uint32_t pi = 0; pi < 1024; pi++)
	// 	{
	// 		if(tableMapped[pi] == 0)
	// 			continue;
	//
	// 		g_physical_address page = G_PAGE_ALIGN_DOWN(tableMapped[pi]);
	// 		memoryPhysicalFree(page);
	// 	}
	//
	// 	g_physical_address table = G_PAGE_ALIGN_DOWN(directoryCurrent[ti]);
	// 	memoryPhysicalFree(table);
	// }

	taskingMemoryTemporarySwitchBack(returnDirectory);

	memoryPhysicalFree(directory);
}

void taskingMemoryInitializeTls(g_task* task)
{
	// Kernel thread-local storage
	if(!task->threadLocal.kernelThreadLocal)
	{
		auto kernelThreadLocal = (g_kernel_threadlocal*) heapAllocate(sizeof(g_kernel_threadlocal));
		kernelThreadLocal->processor = processorGetCurrentId();
		task->threadLocal.kernelThreadLocal = kernelThreadLocal;
	}

	// User thread-local storage from binaries
	if(!task->threadLocal.userThreadLocal)
	{
		g_process* process = task->process;
		if(process->tlsMaster.location)
		{
			// Allocate required virtual range
			uint32_t requiredSize = process->tlsMaster.size;
			uint32_t requiredPages = G_PAGE_ALIGN_UP(requiredSize) / G_PAGE_SIZE;
			if(requiredPages < 1)
				requiredPages = 1;

			g_virtual_address tlsStart = addressRangePoolAllocate(process->virtualRangePool, requiredPages);
			g_virtual_address tlsEnd = tlsStart + requiredPages * G_PAGE_SIZE;

			for(g_virtual_address page = tlsStart; page < tlsEnd; page += G_PAGE_SIZE)
			{
				g_physical_address phys = memoryPhysicalAllocate();
				pagingMapPage(page, phys, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT);
			}

			// Copy TLS contents
			memorySetBytes((void*) tlsStart, 0, process->tlsMaster.size);
			memoryCopy((void*) tlsStart, (void*) process->tlsMaster.location, process->tlsMaster.size);

			// Store information
			task->threadLocal.userThreadLocal = (g_user_threadlocal*) (tlsStart + process->tlsMaster.userThreadOffset);
			task->threadLocal.userThreadLocal->self = task->threadLocal.userThreadLocal;
			task->threadLocal.start = tlsStart;
			task->threadLocal.end = tlsEnd;

			logDebug("%! created tls copy in process %i, thread %i at %h", "threadmgr", process->id, task->id,
			         task->threadLocal.start);
		}
	}
}

void taskingMemoryDestroyTls(g_task* task)
{
	if(task->threadLocal.start)
	{
		for(g_virtual_address page = task->threadLocal.start; page < task->threadLocal.end; page += G_PAGE_SIZE)
		{
			g_physical_address pagePhys = pagingVirtualToPhysical(page);
			if(pagePhys > 0)
			{
				memoryPhysicalFree(pagePhys);
				pagingUnmapPage(page);
			}
		}
		addressRangePoolFree(task->process->virtualRangePool, task->threadLocal.start);
	}

	heapFree(task->threadLocal.kernelThreadLocal);
}

g_physical_address taskingMemoryTemporarySwitchTo(g_physical_address pageDirectory)
{
	g_physical_address back = pagingGetCurrentSpace();
	g_tasking_local* local = taskingGetLocal();
	if(local->scheduling.current)
	{
		if(local->scheduling.current->overridePageDirectory != 0)
			panic("%! %i tried temporary directory switching twice", "tasking", local->scheduling.current->id);

		local->scheduling.current->overridePageDirectory = pageDirectory;
	}
	pagingSwitchToSpace(pageDirectory);
	return back;
}

void taskingMemoryTemporarySwitchBack(g_physical_address back)
{
	g_tasking_local* local = taskingGetLocal();
	if(local->scheduling.current)
		local->scheduling.current->overridePageDirectory = 0;
	pagingSwitchToSpace(back);
}

bool taskingMemoryHandleStackOverflow(g_task* task, g_virtual_address accessed)
{
	g_virtual_address accessedPage = G_PAGE_ALIGN_DOWN(accessed);

	// Is within range of the stack?
	if(accessedPage < task->stack.start || accessedPage > task->stack.end)
	{
		return false;
	}

	// If guard page was accessed, let the task fault
	if(accessedPage < task->stack.start + G_PAGE_SIZE)
	{
		logInfo("%! task %i page-faulted due to overflowing into stack guard page", "pagefault", task->id);
		return false;
	}

	// Extend the stack
	uint32_t tableFlags, pageFlags;
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		tableFlags = G_PAGE_TABLE_KERNEL_DEFAULT;
		pageFlags = G_PAGE_KERNEL_DEFAULT;
	}
	else
	{
		tableFlags = G_PAGE_TABLE_USER_DEFAULT;
		pageFlags = G_PAGE_USER_DEFAULT;
	}

	pagingMapPage(accessedPage, memoryPhysicalAllocate(), tableFlags, pageFlags);
	return true;
}
