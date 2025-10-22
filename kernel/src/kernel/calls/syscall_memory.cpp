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

#include "kernel/calls/syscall_memory.hpp"
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/memory/constants.hpp"

#include "kernel/logger/logger.hpp"

void syscallSbrk(g_task* task, g_syscall_sbrk* data)
{
	data->successful = taskingMemoryExtendHeap(task, data->amount, &data->address);
}

void syscallLowerMemoryAllocate(g_task* task, g_syscall_lower_malloc* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
	{
		data->result = 0;
		return;
	}
	data->result = lowerHeapAllocate(data->size);
}

void syscallLowerMemoryFree(g_task* task, g_syscall_lower_free* data)
{
	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER)
	{
		return;
	}
	lowerHeapFree(data->memory);
}

void syscallAllocateMemory(g_task* task, g_syscall_alloc_mem* data)
{
	data->virtualResult = nullptr;

	uint32_t pages = G_PAGE_ALIGN_UP(data->size) / G_PAGE_SIZE;
	if(pages == 0)
	{
		logInfo("%! task %i failed to allocate an empty physical memory area", "syscall", task->id);
		return;
	}

	g_virtual_address mapped = addressRangePoolAllocate(task->process->virtualRangePool, pages);
	if(mapped == 0)
	{
		logInfo("%! task %i failed to allocate a virtual address range for memory mapping", "syscall", task->id);
		return;
	}

	bool failedPhysical = false;
	for(uint32_t i = 0; i < pages; i++)
	{
		g_physical_address page = memoryPhysicalAllocate();
		if(!page)
		{
			failedPhysical = true;
			break;
		}
		pagingMapPage(mapped + i * G_PAGE_SIZE, page, G_PAGE_TABLE_USER_DEFAULT,G_PAGE_TABLE_USER_DEFAULT,
		              G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT);
	}

	if(failedPhysical)
	{
		logInfo("%! ran out of physical memory during allocate-memory syscall in %i", "syscall", task->id);
		for(uint32_t i = 0; i < pages; i++)
		{
			g_physical_address page = pagingVirtualToPhysical(mapped + i * G_PAGE_SIZE);
			memoryPhysicalFree(page);
		}
		addressRangePoolFree(task->process->virtualRangePool, mapped);
		return;
	}

	data->virtualResult = (void*) mapped;
}

void syscallUnmap(g_task* task, g_syscall_unmap* data)
{
	g_address_range* range = addressRangePoolFind(task->process->virtualRangePool, data->virtualBase);
	if(!range)
		return;

	for(uint32_t i = 0; i < range->pages; i++)
	{
		g_virtual_address virt = range->base + i * G_PAGE_SIZE;
		g_physical_address page = pagingVirtualToPhysical(virt);
		if(!page)
			continue;

		if((range->flags & G_PROC_VIRTUAL_RANGE_FLAG_WEAK) == 0)
			memoryPhysicalFree(page);

		pagingUnmapPage(virt);
	}

	addressRangePoolFree(task->process->virtualRangePool, range->base);
}

void syscallShareMemory(g_task* task, g_syscall_share_mem* data)
{
	data->virtualAddress = 0;

	g_task* targetTask = taskingGetById(data->processId);
	if(!targetTask)
	{
		logInfo("%! task %i was unable to share memory with non-existing process %i", "syscall", task->id,
		        data->processId);
		return;
	}
	g_process* targetProcess = targetTask->process;
	mutexAcquire(&targetProcess->lock);

	g_virtual_address memory = (g_virtual_address) data->memory;
	uint32_t pages = G_PAGE_ALIGN_UP(data->size) / G_PAGE_SIZE;
	if(memory > G_MEM_LOWER_HALF_END || (memory + pages * G_PAGE_SIZE) > G_MEM_LOWER_HALF_END)
	{
		logInfo("%! task %i was unable to share memory because addresses above %h are not allowed", "syscall", task->id,
		        G_MEM_LOWER_HALF_END);
		return;
	}

	g_virtual_address virtualRangeBase = addressRangePoolAllocate(targetProcess->virtualRangePool, pages,
	                                                              G_PROC_VIRTUAL_RANGE_FLAG_NONE);
	mutexRelease(&targetProcess->lock);

	if(virtualRangeBase == 0)
	{
		logInfo(
				"%! task %i was unable to share memory area %h of size %h with task %i because there was no free virtual range",
				"syscall",
				task->id, memory, pages * G_PAGE_SIZE, targetProcess->main->id);
		return;
	}

	for(uint32_t i = 0; i < pages; i++)
	{
		g_physical_address physicalAddr = pagingVirtualToPhysical(memory + i * G_PAGE_SIZE);

		targetTask = taskingGetById(data->processId);
		if(!targetTask)
		{
			logInfo("%! task %i was unable to share memory with no longer existing process %i", "syscall", task->id,
			        data->processId);
			return;
		}

		mutexAcquire(&targetProcess->lock);
		targetProcess = targetTask->process;

		g_physical_address back = taskingMemoryTemporarySwitchTo(targetProcess->pageSpace);
		pagingMapPage(virtualRangeBase + i * G_PAGE_SIZE, physicalAddr, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT);
		taskingMemoryTemporarySwitchBack(back);
		mutexRelease(&targetProcess->lock);

		pageReferenceTrackerIncrement(physicalAddr);
	}

	data->virtualAddress = (void*) virtualRangeBase;
	logDebug("%! shared memory area of process %i at %h of size %h with process %i to address %h", "syscall", task->id,
	         memory,
	         pages * G_PAGE_SIZE, targetProcess->main->id, virtualRangeBase);
}

void syscallMapMmioArea(g_task* task, g_syscall_map_mmio* data)
{
	uint32_t pages = G_PAGE_ALIGN_UP(data->size) / G_PAGE_SIZE;

	g_virtual_address virtualRangeBase = addressRangePoolAllocate(task->process->virtualRangePool, pages,
	                                                              G_PROC_VIRTUAL_RANGE_FLAG_WEAK);
	if(virtualRangeBase == 0)
	{
		logInfo("%! task %i failed to map mmio memory, could not allocate virtual range", "syscall", task->id);
		return;
	}

	for(uint32_t i = 0; i < pages; i++)
	{
		pagingMapPage(virtualRangeBase + i * G_PAGE_SIZE, data->physicalAddress + i * G_PAGE_SIZE,
		              G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT);
	}

	data->virtualAddress = (void*) virtualRangeBase;
}
