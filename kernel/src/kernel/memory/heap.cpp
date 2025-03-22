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
#include "kernel/memory/allocator.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/paging.hpp"
#include "shared/memory/constants.hpp"
#include "shared/panic.hpp"
#include "shared/system/mutex.hpp"

static g_allocator heapAllocator;
static g_virtual_address heapStart = 0;
static g_virtual_address heapEnd = 0;
static uint32_t heapAmountInUse = 0;
static bool heapInitialized = false;

static g_mutex heapLock;

bool _heapExpand();
void _heapMapInitialArea();

void heapInitialize()
{
	if(heapInitialized)
		panic("%! tried to initialized kernel heap twice", "kernheap");

	_heapMapInitialArea();
	mutexInitializeGlobal(&heapLock, __func__);
	memoryAllocatorInitialize(&heapAllocator, G_ALLOCATOR_TYPE_HEAP, heapStart, heapEnd);

	logDebug("%! initialized with area: %h - %h", "heap", heapStart, heapEnd);
	heapInitialized = true;
}

void _heapMapInitialArea()
{
	heapStart = G_MEM_HEAP_START;
	heapEnd = heapStart + G_MEM_HEAP_INITIAL_SIZE;

	g_address virt = heapStart;
	while(virt < heapEnd)
	{
		g_physical_address addr = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
		pagingMapPage(virt, addr, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT);
		virt += G_PAGE_SIZE;
	}
}

void* heapAllocate(uint32_t size)
{
	mutexAcquire(&heapLock);

	if(!heapInitialized)
		panic("%! tried to use uninitialized kernel heap", "kernheap");

	void* ptr = memoryAllocatorAllocate(&heapAllocator, size);
	if(!ptr)
	{
		if(_heapExpand())
		{
			mutexRelease(&heapLock);
			return heapAllocate(size);
		}

		panic("%! failed to allocate kernel memory", "kernheap");
		return nullptr;
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
		panic("%! tried to use uninitialized kernel heap", "kernheap");

	if(!ptr)
	{
		logWarn("%! tried to free nullptr", "kernheap");
		return;
	}

	mutexAcquire(&heapLock);

	heapAmountInUse -= memoryAllocatorFree(&heapAllocator, ptr);

	mutexRelease(&heapLock);
}

uint32_t heapGetUsedAmount()
{
	return heapAmountInUse;
}

bool _heapExpand()
{
	for(g_virtual_address virt = heapEnd; virt < heapEnd + G_MEM_KERN_HEAP_EXPAND_STEP; virt += G_PAGE_SIZE)
	{
		g_physical_address phys = memoryPhysicalAllocate(true);
		if(phys == 0)
		{
			logWarn("%! failed to expand kernel heap, out of physical memory", "kernheap");
			return false;
		}

		pagingMapPage(virt, phys, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT);
	}

	memoryAllocatorExpand(&heapAllocator, G_MEM_KERN_HEAP_EXPAND_STEP);
	heapEnd += G_MEM_KERN_HEAP_EXPAND_STEP;

	logDebug("%! expanded to end %h (%ikb in use)", "kernheap", heapEnd, heapAmountInUse / 1024);

	return true;
}
