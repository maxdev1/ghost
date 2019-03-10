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
	g_virtual_address brk_old = process->heap.brk;
	g_virtual_address brk_new = brk_old + amount;

	// heap expansion is limited
	bool success = false;
	if(brk_new >= G_CONST_USER_MAXIMUM_HEAP_BREAK)
	{
		*outAddress = -1;

	} else
	{
		// expand if necessary
		g_virtual_address virt_above;
		while(brk_new > (virt_above = process->heap.start + process->heap.pages * G_PAGE_SIZE))
		{
			g_physical_address phys = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
			pagingMapPage(virt_above, phys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			pageReferenceTrackerIncrement(phys);
			++process->heap.pages;
		}

		// shrink if possible
		g_virtual_address virtAligned;
		while(brk_new < (virtAligned = process->heap.start + process->heap.pages * G_PAGE_SIZE - G_PAGE_SIZE))
		{
			g_physical_address phys = pagingVirtualToPhysical(virtAligned);
			pagingUnmapPage(virtAligned);
			if(pageReferenceTrackerDecrement(phys) == 0)
			{
				bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, phys);
			}
			--process->heap.pages;
		}

		process->heap.brk = brk_new;
		*outAddress = brk_old;
		success = true;
	}

	taskingTemporarySwitchBack(returnDirectory);
	mutexRelease(&process->lock);
	return success;
}
