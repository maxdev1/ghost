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
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "shared/logger/logger.hpp"

bool taskingMemoryExtendHeap(g_task* task, int32_t amount, uint32_t* outAddress)
{
	g_process* process = task->process;
	mutexAcquire(&process->lock);
	g_physical_address returnDirectory = taskingTemporarySwitchToSpace(task->process->pageDirectory);

	// initialize the heap if necessary
	if(process->heap.brk == 0)
	{
		g_virtual_address heapStart = process->image.end;

		g_physical_address phys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pagingMapPage(heapStart, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		pageReferenceTrackerIncrement(phys);

		process->heap.brk = heapStart;
		process->heap.start = heapStart;
		process->heap.pages = 1;
	}

	// calculate new address
	g_virtual_address oldBrk = process->heap.brk;
	g_virtual_address newBrk = oldBrk + amount;

	// heap expansion is limited
	bool success = false;
	if(newBrk >= G_CONST_USER_MAXIMUM_HEAP_BREAK)
	{
		logInfo("%! process %i went out of memory during sbrk", "syscall", process->main->id);
		*outAddress = -1;

	} else
	{
		// expand if necessary
		g_virtual_address virt_above;
		while(newBrk > (virt_above = process->heap.start + process->heap.pages * G_PAGE_SIZE))
		{
			g_physical_address phys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
			pagingMapPage(virt_above, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			pageReferenceTrackerIncrement(phys);
			++process->heap.pages;
		}

		// shrink if possible
		g_virtual_address virtAligned;
		while(newBrk < (virtAligned = process->heap.start + process->heap.pages * G_PAGE_SIZE - G_PAGE_SIZE))
		{
			g_physical_address phys = pagingVirtualToPhysical(virtAligned);
			pagingUnmapPage(virtAligned);
			if(pageReferenceTrackerDecrement(phys) == 0)
			{
				bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, phys);
			}
			--process->heap.pages;
		}

		process->heap.brk = newBrk;
		*outAddress = oldBrk;
		success = true;
	}

	taskingTemporarySwitchBack(returnDirectory);
	mutexRelease(&process->lock);
	return success;
}

void taskingMemoryCreateInterruptStack(g_task* task)
{
	// Interrupt stack
	g_physical_address intPhys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	g_virtual_address intVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	pagingMapPage(intVirt, intPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	pageReferenceTrackerIncrement(intPhys);
	task->interruptStack.start = intVirt;
	task->interruptStack.end = intVirt + G_PAGE_SIZE;
}

void taskingMemoryCreateStacks(g_task* task)
{
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		task->interruptStack.start = 0;
		task->interruptStack.end = 0;
	} else
	{
		taskingMemoryCreateInterruptStack(task);
	}

	// Main stack
	g_physical_address stackPhys = (g_physical_address) bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	pageReferenceTrackerIncrement(stackPhys);
	g_virtual_address stackVirt;
	if(task->securityLevel == G_SECURITY_LEVEL_KERNEL)
	{
		stackVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
		pagingMapPage(stackVirt, stackPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	} else
	{
		stackVirt = addressRangePoolAllocate(task->process->virtualRangePool, 1);
		pagingMapPage(stackVirt, stackPhys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}
	task->stack.start = stackVirt;
	task->stack.end = stackVirt + G_PAGE_SIZE;
}

g_physical_address taskingMemoryCreatePageDirectory()
{
	g_page_directory directoryCurrent = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;

	g_physical_address directoryPhys = (g_physical_address) bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	g_virtual_address directoryTempVirt = addressRangePoolAllocate(memoryVirtualRangePool, 1);
	g_page_directory directoryTemp = (g_page_directory) directoryTempVirt;
	pagingMapPage(directoryTempVirt, directoryPhys);

	// clone kernel space mappings
	for(uint32_t ti = 0; ti < 1024; ti++)
	{
		if(!((directoryCurrent[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE))
			directoryTemp[ti] = directoryCurrent[ti];
		else
			directoryTemp[ti] = 0;
	}

	// clone mappings for the lowest 4 MiB
	directoryTemp[0] = directoryCurrent[0];

	// recursive self-map
	directoryTemp[1023] = directoryPhys | DEFAULT_KERNEL_TABLE_FLAGS;

	pagingUnmapPage(directoryTempVirt);
	return directoryPhys;
}

