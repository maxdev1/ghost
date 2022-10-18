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

#include "kernel/memory/memory.hpp"
#include "kernel/debug/debug_interface.hpp"
#include "kernel/kernel.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/page_reference_tracker.hpp"
#include "kernel/memory/paging.hpp"

g_address_range_pool* memoryVirtualRangePool = 0;

void _memoryRelocatePhysicalBitmap(g_setup_information* setupInformation)
{
	uint32_t bitmapPages = ((setupInformation->bitmapArrayEnd - setupInformation->bitmapArrayStart) / G_PAGE_SIZE);
	g_virtual_address bitmapVirtual = addressRangePoolAllocate(memoryVirtualRangePool, bitmapPages);

	for(uint32_t i = 0; i < bitmapPages; i++)
	{
		g_offset off = (i * G_PAGE_SIZE);
		pagingMapPage(bitmapVirtual + off, setupInformation->bitmapArrayStart + off, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}
	bitmapPageAllocatorRelocate(&memoryPhysicalAllocator, bitmapVirtual);
}

void memoryInitialize(g_setup_information* setupInformation)
{
	bitmapPageAllocatorInitialize(&memoryPhysicalAllocator, (g_bitmap_header*) setupInformation->bitmapArrayStart);
	logInfo("%! available: %i MiB", "memory", (memoryPhysicalAllocator.freePageCount * G_PAGE_SIZE) / 1024 / 1024);

	heapInitialize(setupInformation->heapStart, setupInformation->heapEnd);
	lowerHeapInitialize(G_LOWER_HEAP_MEMORY_START, G_LOWER_HEAP_MEMORY_END);

	memoryVirtualRangePool = (g_address_range_pool*) heapAllocate(sizeof(g_address_range_pool));
	addressRangePoolInitialize(memoryVirtualRangePool);
	addressRangePoolAddRange(memoryVirtualRangePool, G_KERNEL_VIRTUAL_RANGES_START, G_KERNEL_VIRTUAL_RANGES_END);

	pageReferenceTrackerInitialize();

	_memoryRelocatePhysicalBitmap(setupInformation);
}

void memoryUnmapSetupMemory()
{
	for(g_virtual_address addr = G_LOWER_MEMORY_END; addr < G_KERNEL_AREA_START; addr += G_PAGE_SIZE)
		pagingUnmapPage(addr);
}

g_physical_address memoryPhysicalAllocate(bool untracked)
{
	g_physical_address page = bitmapPageAllocatorAllocate(&memoryPhysicalAllocator);
	if(!untracked && page)
		pageReferenceTrackerIncrement(page);
	return page;
}

void memoryPhysicalFree(g_physical_address page)
{
	if(!page)
		return;
	if(pageReferenceTrackerDecrement(page) == 0)
		bitmapPageAllocatorMarkFree(&memoryPhysicalAllocator, page);
}

g_virtual_address memoryAllocateKernelRange(int32_t pages)
{
	g_virtual_address virt = addressRangePoolAllocate(memoryVirtualRangePool, pages);
	for(int32_t i = 0; i < pages; i++)
	{
		g_physical_address phys = memoryPhysicalAllocate();
		pagingMapPage(virt + (i * G_PAGE_SIZE), phys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}
	return virt;
}

void memoryFreeKernelRange(g_virtual_address address)
{
	auto range = addressRangePoolFind(memoryVirtualRangePool, address);
	if(!range)
	{
		logWarn("%! tried to free unallocated kernel range at %x", "memory", address);
		return;
	}

	for(int32_t i = 0; i < range->pages; i++)
	{
		g_virtual_address virt = range->base + (i * G_PAGE_SIZE);
		g_physical_address phys = pagingVirtualToPhysical(virt);
		pagingUnmapPage(virt);
		memoryPhysicalFree(phys);
	}

	addressRangePoolFree(memoryVirtualRangePool, address);
}