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
#include "kernel/memory/constants.hpp"
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/tasking/tasking_memory.hpp"

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
	const uint64_t size = data->size;
	data->virtualResult = nullptr;
	data->physicalResult = nullptr;

	uint32_t pages;
	if(!memorySizeToPageCount(size, &pages))
		return;

	auto process = task->process;
	mutexAcquire(&process->lock);
	auto mapped = addressRangePoolAllocate(process->virtualRangePool, pages);
	if(!mapped)
	{
		mutexRelease(&process->lock);
		return;
	}

	g_physical_address page = 0;
	uint32_t installed = 0;
	for(; installed < pages; ++installed)
	{
		page = memoryPhysicalTryAllocate();
		if(!page)
			break;
		auto address = mapped + installed * G_PAGE_SIZE;
		memorySetBytes((void*) G_MEM_PHYS_TO_VIRT(page), 0, G_PAGE_SIZE);
		if(!pagingMapPage(address, page, G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT))
		{
			memoryPhysicalFree(page);
			break;
		}
	}

	if(installed != pages)
	{
		// Remove only our installed prefix before returning any frames to the allocator.
		for(uint32_t i = 0; i < installed; ++i)
		{
			auto address = mapped + i * G_PAGE_SIZE;
			auto physical = pagingVirtualToPhysical(address);
			pagingUnmapPage(address);
			memoryPhysicalFree(physical);
		}
		addressRangePoolFree(process->virtualRangePool, mapped);
	}
	else
	{
		data->virtualResult = (void*) mapped;
		data->physicalResult = (task->securityLevel <= G_SECURITY_LEVEL_DRIVER && pages == 1) ? (void*) page : nullptr;
	}
	mutexRelease(&process->lock);
}

void syscallUnmap(g_task* task, g_syscall_unmap* data)
{
	auto process = task->process;
	mutexAcquire(&process->lock);
	auto range = addressRangePoolFind(process->virtualRangePool, data->virtualBase);
	if(range && range->used && !(range->flags & G_PROC_VIRTUAL_RANGE_FLAG_INTERNAL))
	{
		for(uint32_t i = 0; i < range->pages; i++)
		{
			g_virtual_address virt = range->base + i * G_PAGE_SIZE;
			uint64_t entry = pagingVirtualToPageEntry(virt);
			if(!(entry & G_PAGE_PRESENT))
				continue;
			pagingUnmapPage(virt);
			if(!(entry & G_PAGE_UNMANAGED_FLAG))
				memoryPhysicalFree(entry & G_PAGE_PHYSICAL_MASK);
		}
		addressRangePoolFree(process->virtualRangePool, range->base);
	}
	mutexRelease(&process->lock);
}

struct g_shared_page
{
	g_physical_address address;
	uint64_t flags;
};

void syscallShareMemory(g_task* task, g_syscall_share_mem* data)
{
	const g_virtual_address memory = (g_virtual_address) data->memory;
	const uint64_t size = data->size;
	const g_tid targetId = data->processId;
	data->virtualAddress = nullptr;
	uint32_t pages;
	if(size > G_USER_VIRTUAL_RANGES_END - G_USER_VIRTUAL_RANGES_START ||
	   !memorySizeToPageCount(size, &pages))
		return;
	const uint64_t mappedSize = (uint64_t) pages * G_PAGE_SIZE;
	if((memory & G_PAGE_ALIGN_MASK) || memory > G_MEM_LOWER_HALF_END ||
	   mappedSize > G_MEM_LOWER_HALF_END - memory + 1)
		return;

	auto target = taskingAcquireTask(targetId);
	if(!target)
		return;
	auto process = target->process;
	auto snapshot = (g_shared_page*) heapTryAllocate(sizeof(g_shared_page) * pages);
	if(!snapshot)
	{
		taskingReleaseTask(target);
		return;
	}
	uint32_t retained = 0;
	bool allUnmanaged = true;

	// Retain the source while its mappings are locked, before publishing aliases.
	// Never hold both processes' locks, so reciprocal shares cannot deadlock.
	mutexAcquire(&task->process->lock);
	for(; retained < pages; ++retained)
	{
		uint64_t entry = pagingVirtualToPageEntry(memory + retained * G_PAGE_SIZE);
		if(!(entry & G_PAGE_PRESENT))
			break;
		snapshot[retained] = {entry & G_PAGE_PHYSICAL_MASK, entry & G_PAGE_UNMANAGED_FLAG};
		if(!(entry & G_PAGE_UNMANAGED_FLAG))
		{
			pageReferenceTrackerIncrement(snapshot[retained].address);
			allUnmanaged = false;
		}
	}
	mutexRelease(&task->process->lock);

	g_virtual_address base = 0;
	if(retained == pages)
	{
		mutexAcquire(&process->lock);
		if(!process->terminating)
			base = addressRangePoolAllocate(process->virtualRangePool, pages,
											allUnmanaged ? G_PROC_VIRTUAL_RANGE_FLAG_WEAK
														 : G_PROC_VIRTUAL_RANGE_FLAG_NONE);
		if(base)
		{
			auto back = taskingMemoryTemporarySwitchTo(process->pageSpace);
			uint32_t mapped = 0;
			for(; mapped < pages; ++mapped)
				if(!pagingMapPage(base + mapped * G_PAGE_SIZE, snapshot[mapped].address, G_PAGE_TABLE_USER_DEFAULT,
								  G_PAGE_USER_DEFAULT | snapshot[mapped].flags))
					break;
			if(mapped != pages)
			{
				for(uint32_t i = 0; i < mapped; ++i)
					pagingUnmapPage(base + i * G_PAGE_SIZE);
				addressRangePoolFree(process->virtualRangePool, base);
				base = 0;
			}
			taskingMemoryTemporarySwitchBack(back);
		}
		mutexRelease(&process->lock);
	}
	if(!base)
		for(uint32_t i = 0; i < retained; ++i)
			if(!(snapshot[i].flags & G_PAGE_UNMANAGED_FLAG))
				memoryPhysicalFree(snapshot[i].address);
	heapFree(snapshot);
	data->virtualAddress = (void*) base;
	taskingReleaseTask(target);
}

void syscallMapMmioArea(g_task* task, g_syscall_map_mmio* data)
{
	const g_physical_address physicalAddress = data->physicalAddress;
	const uint64_t size = data->size;
	data->virtualAddress = nullptr;

	if(task->securityLevel > G_SECURITY_LEVEL_DRIVER || size == 0 || (physicalAddress & G_PAGE_ALIGN_MASK))
		return;

	const uint64_t mappedSize = G_PAGE_ALIGN_UP(size);
	if(!memoryPhysicalIsRangeAddressable(physicalAddress, mappedSize))
		return;

	const uint32_t pages = mappedSize / G_PAGE_SIZE;
	auto pool = task->process->virtualRangePool;
	mutexAcquire(&task->process->lock);
	g_virtual_address virtualRangeBase = addressRangePoolAllocate(pool, pages, G_PROC_VIRTUAL_RANGE_FLAG_WEAK);
	if(virtualRangeBase == 0)
	{
		logInfo("%! task %i failed to map mmio memory, could not allocate virtual range", "syscall", task->id);
		mutexRelease(&task->process->lock);
		return;
	}

	for(uint32_t i = 0; i < pages; i++)
	{
		if(!pagingMapPage(virtualRangeBase + i * G_PAGE_SIZE, physicalAddress + i * G_PAGE_SIZE,
						  G_PAGE_TABLE_USER_DEFAULT, G_PAGE_USER_DEFAULT | G_PAGE_UNMANAGED_FLAG))
		{
			// Only undo mappings installed by this call. Device pages are never freed.
			for(uint32_t mapped = 0; mapped < i; mapped++)
				pagingUnmapPage(virtualRangeBase + mapped * G_PAGE_SIZE);
			addressRangePoolFree(pool, virtualRangeBase);
			mutexRelease(&task->process->lock);
			return;
		}
	}

	data->virtualAddress = (void*) virtualRangeBase;
	mutexRelease(&task->process->lock);
}
