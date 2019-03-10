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

#include "kernel/memory/lower_heap.hpp"
#include "kernel/kernel.hpp"
#include "kernel/memory/paging.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/chunk_allocator.hpp"
#include "shared/memory/bitmap_page_allocator.hpp"

static g_chunk_allocator lowerHeapAllocator;
static g_virtual_address lowerHeapStart = 0;
static g_virtual_address lowerHeapEnd = 0;
static uint32_t lowerHeapAmountInUse = 0;
static bool lowerHeapInitialized = false;

void lowerHeapInitialize(g_virtual_address start, g_virtual_address end)
{
	chunkAllocatorInitialize(&lowerHeapAllocator, start, end);
	lowerHeapStart = start;
	lowerHeapEnd = end;

	logDebug("%! initialized with area: %h - %h", "lowerheap", start, end);
	lowerHeapInitialized = true;
}

void* lowerHeapAllocate(uint32_t size)
{
	if(!lowerHeapInitialized)
		kernelPanic("%! tried to use uninitialized kernel heap", "lowerheap");

	void* ptr = chunkAllocatorAllocate(&lowerHeapAllocator, size);
	if(!ptr)
		kernelPanic("%! failed to allocate kernel memory", "lowerheap");

	lowerHeapAmountInUse += size;
	return ptr;
}

void lowerHeapFree(void* ptr)
{
	if(!lowerHeapInitialized)
		kernelPanic("%! tried to use uninitialized lower heap", "lowerheap");

	lowerHeapAmountInUse -= chunkAllocatorFree(&lowerHeapAllocator, ptr);
}

uint32_t lowerHeapGetUsedAmount()
{
	return lowerHeapAmountInUse;
}
