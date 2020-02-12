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

#include "kernel/memory/heap.hpp"
#include "kernel/kernel.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/chunk_allocator.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"
#include "shared/system/mutex.hpp"

static g_chunk_allocator heapAllocator;
static g_virtual_address heapStart = 0;
static g_virtual_address heapEnd = 0;
static uint32_t heapAmountInUse = 0;
static bool heapInitialized = false;

static g_mutex heapLock;

void heapInitialize(g_virtual_address start, g_virtual_address end)
{
	mutexInitialize(&heapLock);
	mutexAcquire(&heapLock);

	chunkAllocatorInitialize(&heapAllocator, start, end);
	heapStart = start;
	heapEnd = end;

	logDebug("%! initialized with area: %h - %h", "kernheap", start, end);
	heapInitialized = true;

	mutexRelease(&heapLock);
}

bool heapExpand()
{
	if(heapEnd + G_CONST_KERNEL_HEAP_EXPAND_STEP > G_CONST_KERNEL_HEAP_END)
	{
		logDebug("%! out of virtual memory area to map", "kernheap");
		return false;
	}

	for(g_virtual_address v = heapEnd; v < heapEnd + G_CONST_KERNEL_HEAP_EXPAND_STEP; v += G_PAGE_SIZE)
	{
		g_physical_address p = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		if(p == 0)
		{
			logWarn("%! failed to expand kernel heap, out of physical memory", "kernheap");
			return false;
		}

		pagingMapPage(v, p, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}

	chunkAllocatorExpand(&heapAllocator, G_CONST_KERNEL_HEAP_EXPAND_STEP);
	heapEnd += G_CONST_KERNEL_HEAP_EXPAND_STEP;

	logDebug("%! expanded to end %h (%ikb in use)", "kernheap", heapEnd, heapAmountInUse / 1024);

	return true;
}

void* heapAllocate(uint32_t size)
{
	mutexAcquire(&heapLock);

	if(!heapInitialized)
		kernelPanic("%! tried to use uninitialized kernel heap", "kernheap");

	void* ptr = chunkAllocatorAllocate(&heapAllocator, size);
	if(!ptr)
	{
		if(heapExpand())
		{
			mutexRelease(&heapLock);
			return heapAllocate(size);
		}

		kernelPanic("%! failed to allocate kernel memory", "kernheap");
		return 0;
	}

	heapAmountInUse += size;
	mutexRelease(&heapLock);
	return ptr;
}

void* heapAllocateClear(uint32_t size)
{
	void* ptr = heapAllocate(size);
	memorySetBytes(ptr, 0, size);
	return ptr;
}

void heapFree(void* ptr)
{
	if(!heapInitialized)
		kernelPanic("%! tried to use uninitialized kernel heap", "kernheap");

	mutexAcquire(&heapLock);
	heapAmountInUse -= chunkAllocatorFree(&heapAllocator, ptr);
	mutexRelease(&heapLock);
}

uint32_t heapGetUsedAmount()
{
	return heapAmountInUse;
}

void* operator new(size_t size)
{
	return heapAllocate(size);
}

void* operator new[](size_t size)
{
	return heapAllocate(size);
}

void operator delete(void* m)
{
	heapFree(m);
}

void operator delete[](void* m)
{
	heapFree(m);
}
