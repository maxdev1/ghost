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

#include "ghost/signal.h"

#include "kernel/calls/syscall_memory.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"

#include "shared/logger/logger.hpp"

void syscallSbrk(g_task* task, g_syscall_sbrk* data)
{
	data->successful = taskingMemoryExtendHeap(task, data->amount, &data->address);
}

void syscallLowerMemoryAllocate(g_task* task, g_syscall_lower_malloc* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER) {
		data->result = 0;
		return;
	}
	data->result = lowerHeapAllocate(data->size);
}

void syscallLowerMemoryFree(g_task* task, g_syscall_lower_free* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER) {
		return;
	}
	lowerHeapFree(data->memory);
}

void syscallAllocateMemory(g_task* task, g_syscall_alloc_mem* data)
{
	data->virtualResult = 0;

	uint32_t pages = G_PAGE_ALIGN_UP(data->size);
	if(pages == 0) return;

	/* Prepare a virtual range */
	g_virtual_address mapped = addressRangePoolAllocate(task->process->virtualRangePool, pages);
	if(mapped == 0) return;

	/* Try to allocate physical memory */
	bool failedPhysical = false;
	for (uint32_t i = 0; i < pages; i++)
	{
		g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		if(!page)
		{
			failedPhysical = true;
			break;
		}
		pagingMapPage(mapped + i * G_PAGE_SIZE, page, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		pageReferenceTrackerIncrement(page);
	}

	/* If physical mapping fails, clean up allocated memory */
	if(failedPhysical)
	{
		logInfo("%! ran out of physical memory during allocate-memory syscall in %i", "syscall", task->id);
		for(uint32_t i = 0; i < pages; i++)
		{
			g_physical_address page = pagingVirtualToPhysical(mapped + i * G_PAGE_SIZE);
			if(page)
			{
				bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, page);
				pageReferenceTrackerDecrement(page);
			}
		}
		addressRangePoolFree(task->process->virtualRangePool, mapped);
		return;
	}

	/* Mapping successful */
	data->virtualResult = (void*) mapped;
}

void syscallUnmap(g_task* task, g_syscall_unmap* data)
{
	g_address_range* range = addressRangePoolFind(task->process->virtualRangePool, data->virtualBase);
	if(!range) return;

	/* Unmap all pages in the range */
	for(uint32_t i = 0; i < range->pages; i++)
	{
		g_virtual_address virt = range->base + i * G_PAGE_SIZE;
		g_physical_address page = pagingVirtualToPhysical(virt);
		if(!page) continue;

		/* Free physical memory if possible */
		if((range->flags & G_PROC_VIRTUAL_RANGE_FLAG_WEAK) == 0 && pageReferenceTrackerDecrement(page) == 0)
			bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, page);

		pagingUnmapPage(virt);
	}

	/* Free range */
	addressRangePoolFree(task->process->virtualRangePool, range->base);
}

void syscallShareMemory(g_task* task, g_syscall_share_mem* data)
{
	data->virtualAddress = 0;

	/* Find target thread */
	g_task* targetThread = taskingGetById(data->processId);
	if (!targetThread) {
		logInfo("%! task %i was unable to share memory with non-existing process %i", "syscall", task->id, data->processId);
		return;
	}
	g_process* targetProcess = targetThread->process;

	/* Calculate and check validity */
	g_virtual_address memory = (g_virtual_address) data->memory;
	uint32_t pages = G_PAGE_ALIGN_UP(data->size) / G_PAGE_SIZE;
	if (memory > G_CONST_KERNEL_AREA_START || (memory + pages * G_PAGE_SIZE) > G_CONST_KERNEL_AREA_START)
	{
		logInfo("%! task %i was unable to share memory because addresses above %h are not allowed", "syscall", task->id, G_CONST_KERNEL_AREA_START);
		return;
	}

	/* Allocate a virtual range in the target process */
	g_virtual_address virtualRangeBase = addressRangePoolAllocate(targetProcess->virtualRangePool, pages, G_PROC_VIRTUAL_RANGE_FLAG_NONE);
	if (virtualRangeBase == 0)
	{
		logInfo("%! task %i was unable to share memory area %h of size %h with task %i because there was no free virtual range", "syscall",
				task->id, memory, pages * G_PAGE_SIZE, targetProcess->main->id);
		return;
	}

	/* Switch into target space and map pages */
	g_physical_address back = taskingTemporarySwitchToSpace(targetProcess->pageDirectory);
	for (uint32_t i = 0; i < pages; i++) {
		g_physical_address physicalAddr = pagingVirtualToPhysical(memory + i * G_PAGE_SIZE);
		pagingMapPage(virtualRangeBase + i * G_PAGE_SIZE, physicalAddr, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
		pageReferenceTrackerIncrement(physicalAddr);
	}
	taskingTemporarySwitchBack(back);

	/* Mapping successful */
	data->virtualAddress = (void*) virtualRangeBase;
	logDebug("%! shared memory area of process %i at %h of size %h with process %i to address %h", "syscall", task->id, memory,
			pages * G_PAGE_SIZE, targetProcess->main->id, virtualRangeBase);
}

void syscallMapMmioArea(g_task* task, g_syscall_map_mmio* data)
{
	uint32_t pages = G_PAGE_ALIGN_UP(data->size) / G_PAGE_SIZE;

	/* Allocate a weak virtual range */
	g_virtual_address virtualRangeBase = addressRangePoolAllocate(task->process->virtualRangePool, pages, G_PROC_VIRTUAL_RANGE_FLAG_WEAK);
	if (virtualRangeBase == 0)
	{
		logInfo("%! task %i failed to map mmio memory, could not allocate virtual range", "syscall", task->id);
		return;
	}

	/* Map to physical memory */
	for(uint32_t i = 0; i < pages; i++)
	{
		pagingMapPage(virtualRangeBase + i * G_PAGE_SIZE, data->physicalAddress + i * G_PAGE_SIZE, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	}

	data->virtualAddress = (void*) virtualRangeBase;
}
