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
#include "kernel/memory/heap.hpp"
#include "kernel/memory/lower_heap.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/kernel.hpp"
#include "kernel/debug/debug_interface.hpp"
#include "kernel/memory/page_reference_tracker.hpp"

g_bitmap_page_allocator memoryPhysicalAllocator;
static g_bitmap_entry memoryPhysicalBitmap[G_BITMAP_SIZE];
g_address_range_pool* memoryVirtualRangePool = 0;

void memoryInitialize(g_setup_information* setupInformation)
{
	memoryInitializePhysicalAllocator(setupInformation);
	heapInitialize(setupInformation->heapStart, setupInformation->heapEnd);
	lowerHeapInitialize(G_CONST_LOWER_HEAP_MEMORY_START, G_CONST_LOWER_HEAP_MEMORY_END);

	memoryVirtualRangePool = (g_address_range_pool*) heapAllocate(sizeof(g_address_range_pool));
	addressRangePoolInitialize(memoryVirtualRangePool);
	addressRangePoolAddRange(memoryVirtualRangePool, G_CONST_KERNEL_VIRTUAL_RANGES_START, G_CONST_KERNEL_VIRTUAL_RANGES_END);

	pageReferenceTrackerInitialize();
}

void memoryInitializePhysicalAllocator(g_setup_information* setupInformation)
{
	uint32_t bitmapSize = setupInformation->bitmapEnd - setupInformation->bitmapStart;
	if(bitmapSize != G_BITMAP_SIZE)
		kernelPanic("%! bitmap provided by loader has illegal length: %h, expected: %h", "memoryInitializePhysicalAllocator", bitmapSize, G_BITMAP_SIZE);

	bitmapPageAllocatorInitialize(&memoryPhysicalAllocator, memoryPhysicalBitmap);
	memoryCopy(memoryPhysicalBitmap, (g_bitmap_entry*) setupInformation->bitmapStart, G_BITMAP_SIZE);
	bitmapPageAllocatorRefresh(&memoryPhysicalAllocator);

	G_DEBUG_INTERFACE_SYSTEM_INFORMATION("memory.freePageCount", memoryPhysicalAllocator.freePageCount);
	logInfo("%! available memory: %i MB", "memory", (memoryPhysicalAllocator.freePageCount * G_PAGE_SIZE) / 1024 / 1024);
}

void memoryUnmapSetupMemory()
{
	for(g_virtual_address addr = G_CONST_LOWER_MEMORY_END; addr < G_CONST_KERNEL_AREA_START; addr += G_PAGE_SIZE)
		pagingUnmapPage(addr);
}
