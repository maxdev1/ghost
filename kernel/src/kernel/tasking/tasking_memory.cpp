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
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/system/processor/processor.hpp"
#include "shared/logger/logger.hpp"
#include "shared/panic.hpp"

bool taskingMemoryExtendHeap(g_task* task, int32_t amount, uint32_t* outAddress)
{
	g_process* process = task->process;
	mutexAcquire(&process->lock);
	g_physical_address returnDirectory = taskingMemoryTemporarySwitchTo(task->process->pageDirectory);

	// Initialize the heap if necessary
	if(process->heap.brk == 0)
	{
		g_virtual_address heapStart = process->image.end;

		g_physical_address phys = memoryPhysicalAllocate();
		pagingMapPage(heapStart, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);

		process->heap.brk = heapStart;
		process->heap.start = heapStart;
		process->heap.pages = 1;
	}

	// Calculate new address
	g_virtual_address oldBrk = process->heap.brk;
	g_virtual_address newBrk = oldBrk + amount;

	// Heap expansion is limited
	bool success = false;
	if(newBrk >= G_USER_MAXIMUM_HEAP_BREAK)
	{
		logInfo("%! process %i went out of memory during sbrk", "syscall", process->main->id);
		*outAddress = -1;
	}
	else
	{
		// Expand if necessary
		g_virtual_address virt_above;
		while(newBrk > (virt_above = process->heap.start + process->heap.pages * G_PAGE_SIZE))
		{
			g_physical_address phys = memoryPhysicalAllocate();
			pagingMapPage(virt_above, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
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
	}

	taskingMemoryTemporarySwitchBack(returnDirectory);
	mutexRelease(&process->lock);
	return success;
}

void taskingMemoryCreateStacks(g_task* task)
{
	// Interrupt stack for ring 3 & VM86 tasks
	if(task->securityLevel != G_SECURITY_LEVEL_KERNEL || task->type == G_TASK_TYPE_VM86)
	{
		task->interruptStack = taskingMemoryCreateStack(memoryVirtualRangePool, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS, G_TASKING_MEMORY_INTERRUPT_STACK_PAGES);
	}
	else
	{
		task->interruptStack.start = 0;
		task->interruptStack.end = 0;
	}

	// Create task stack
	if(task->type == G_TASK_TYPE_VM86)
	{
		uint32_t stackSize = 2 * G_PAGE_SIZE;
		task->stack.start = (uint32_t) lowerHeapAllocate(stackSize);
		task->stack.end = task->stack.start + stackSize;
	}
	else if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		task->stack = taskingMemoryCreateStack(memoryVirtualRangePool, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS, G_TASKING_MEMORY_KERNEL_STACK_PAGES);
	}
	else
	{
		task->stack = taskingMemoryCreateStack(task->process->virtualRangePool, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS, G_TASKING_MEMORY_USER_STACK_PAGES);
	}
}

g_stack taskingMemoryCreateStack(g_address_range_pool* addressRangePool, uint32_t tableFlags, uint32_t pageFlags, int pages)
{
	g_virtual_address stackVirt = addressRangePoolAllocate(addressRangePool, pages);

	// Only allocate and map the last page of the stack; when the process faults, lazy-allocate more physical space.
	// The first page of the allocated virtual range is used as a "guard page" and makes the process fault when accessed.
	g_physical_address pagePhys = memoryPhysicalAllocate();
	uint32_t stackEnd = stackVirt + pages * G_PAGE_SIZE;
	pagingMapPage(stackEnd - G_PAGE_SIZE, pagePhys, tableFlags, pageFlags);

	g_stack stack;
	stack.start = stackVirt;
	stack.end = stackEnd;
	return stack;
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

g_physical_address taskingMemoryCreatePageDirectory()
{
	g_page_directory directoryCurrent = (g_page_directory) G_RECURSIVE_PAGE_DIRECTORY_ADDRESS;

	g_physical_address directoryPhys = memoryPhysicalAllocate();
	g_virtual_address directoryTempVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	g_page_directory directoryTemp = (g_page_directory) directoryTempVirt;
	pagingMapPage(directoryTempVirt, directoryPhys);

	// Copy kernel table mappings
	for(uint32_t ti = 0; ti < 1024; ti++)
	{
		if((directoryCurrent[ti] & G_PAGE_TABLE_USERSPACE) == 0)
			directoryTemp[ti] = directoryCurrent[ti];
		else
			directoryTemp[ti] = 0;
	}

	// Copy mappings for the lowest 4 MiB
	directoryTemp[0] = directoryCurrent[0];

	// Recursive self-map
	directoryTemp[1023] = directoryPhys | DEFAULT_KERNEL_TABLE_FLAGS;

	pagingUnmapPage(directoryTempVirt);
	return directoryPhys;
}

void taskingMemoryDestroyPageDirectory(g_physical_address directory)
{
	g_physical_address returnDirectory = taskingMemoryTemporarySwitchTo(directory);

	// Clear mappings and free physical space above 4 MiB
	g_page_directory directoryCurrent = (g_page_directory) G_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	for(uint32_t ti = 1; ti < 1024; ti++)
	{
		if(!(directoryCurrent[ti] & G_PAGE_TABLE_USERSPACE))
			continue;

		g_page_table tableMapped = ((g_page_table) G_RECURSIVE_PAGE_DIRECTORY_AREA) + (0x400 * ti);
		for(uint32_t pi = 0; pi < 1024; pi++)
		{
			if(tableMapped[pi] == 0)
				continue;

			g_physical_address page = G_PAGE_ALIGN_DOWN(tableMapped[pi]);
			memoryPhysicalFree(page);
		}

		g_physical_address table = G_PAGE_ALIGN_DOWN(directoryCurrent[ti]);
		memoryPhysicalFree(table);
	}

	taskingMemoryTemporarySwitchBack(returnDirectory);

	memoryPhysicalFree(directory);
}

void taskingMemoryInitializeTls(g_task* task)
{
	// Kernel thread-local storage
	if(!task->threadLocal.kernelThreadLocal)
	{
		g_kernel_threadlocal* kernelThreadLocal = (g_kernel_threadlocal*) heapAllocate(sizeof(g_kernel_threadlocal));
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

			g_virtual_address tlsStart = addressRangePoolAllocate(process->virtualRangePool, requiredPages);
			g_virtual_address tlsEnd = tlsStart + requiredPages * G_PAGE_SIZE;

			for(g_virtual_address page = tlsStart; page < tlsEnd; page += G_PAGE_SIZE)
			{
				g_physical_address phys = memoryPhysicalAllocate();
				pagingMapPage(page, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			}

			// Copy TLS contents
			memorySetBytes((void*) tlsStart, 0, process->tlsMaster.size);
			memoryCopy((void*) tlsStart, (void*) process->tlsMaster.location, process->tlsMaster.size);

			// Store information
			task->threadLocal.userThreadLocal = (g_user_threadlocal*) (tlsStart + process->tlsMaster.userThreadOffset);
			task->threadLocal.userThreadLocal->self = task->threadLocal.userThreadLocal;
			task->threadLocal.start = tlsStart;
			task->threadLocal.end = tlsEnd;

			logDebug("%! created tls copy in process %i, thread %i at %h", "threadmgr", process->id, task->id, task->threadLocal.start);
		}
	}
}

void taskingMemoryDestroyThreadLocalStorage(g_task* task)
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
		tableFlags = DEFAULT_KERNEL_TABLE_FLAGS;
		pageFlags = DEFAULT_KERNEL_PAGE_FLAGS;
	}
	else
	{
		tableFlags = DEFAULT_USER_TABLE_FLAGS;
		pageFlags = DEFAULT_USER_PAGE_FLAGS;
	}

	pagingMapPage(accessedPage, memoryPhysicalAllocate(), tableFlags, pageFlags);
	return true;
}
