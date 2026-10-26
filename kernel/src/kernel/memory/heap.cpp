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
#include "kernel/memory/constants.hpp"
#include "kernel/panic.hpp"
#include "kernel/system/mutex.hpp"

static g_allocator heapAllocator;
static g_virtual_address heapStart = 0;
static g_virtual_address heapEnd = 0;
static g_size heapAmountInUse = 0;
static bool heapInitialized = false;

static g_mutex heapLock;

bool _heapExpand();
bool _heapExpandWithReserve(uint32_t reservePages);
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

    g_size allocatedSize;
    void* ptr;
    while(!(ptr = memoryAllocatorAllocate(&heapAllocator, size, &allocatedSize)))
    {
        if(!_heapExpand())
        {
            mutexRelease(&heapLock);
            panic("%! failed to allocate critical kernel memory", "kernheap");
            return nullptr;
        }
    }

	heapAmountInUse += allocatedSize;

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

g_size heapGetUsedAmount()
{
	mutexAcquire(&heapLock);
	auto amount = heapAmountInUse;
	mutexRelease(&heapLock);
	return amount;
}

void* heapTryAllocate(uint32_t size)
{
    mutexAcquire(&heapLock);
    g_size capacity;
    void* result;
    while(!(result = memoryAllocatorAllocate(&heapAllocator, size, &capacity)))
    {
        if(!_heapExpandWithReserve(1024))
        {
            mutexRelease(&heapLock);
            return nullptr;
        }
    }
    heapAmountInUse += capacity;
    mutexRelease(&heapLock);
    return result;
}

bool _heapExpand() { return _heapExpandWithReserve(0); }

bool _heapExpandWithReserve(uint32_t reservePages)
{
    uint32_t installed = 0;
    const uint32_t pages = G_MEM_KERN_HEAP_EXPAND_STEP / G_PAGE_SIZE;
    for(; installed < pages; ++installed)
    {
        auto page = bitmapPageAllocatorTryAllocate(&memoryPhysicalAllocator, reservePages);
        if(!page)
            break;
        if(!pagingMapPage(heapEnd + installed * G_PAGE_SIZE, page, G_PAGE_TABLE_KERNEL_DEFAULT, G_PAGE_KERNEL_DEFAULT))
        {
            bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, page);
            break;
        }
    }
    if(installed != pages)
    {
        while(installed)
        {
            auto address = heapEnd + --installed * G_PAGE_SIZE;
            auto page = pagingVirtualToPhysical(address);
            pagingUnmapPage(address);
            bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, page);
        }
        return false;
    }
    memoryAllocatorExpand(&heapAllocator, G_MEM_KERN_HEAP_EXPAND_STEP);
    heapEnd += G_MEM_KERN_HEAP_EXPAND_STEP;
    return true;
}
