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
#include "kernel/memory/allocator.hpp"
#include "kernel/panic.hpp"

static g_allocator lowerHeapAllocator;
static g_mutex lowerHeapMutex;
static g_virtual_address lowerHeapStart = 0;
static g_virtual_address lowerHeapEnd = 0;
static uint32_t lowerHeapAmountInUse = 0;
static bool lowerHeapInitialized = false;

void lowerHeapInitialize(g_virtual_address start, g_virtual_address end)
{
	memoryAllocatorInitialize(&lowerHeapAllocator, G_ALLOCATOR_TYPE_LOWERMEM, start, end);
	lowerHeapStart = start;
	lowerHeapEnd = end;
	mutexInitializeGlobal(&lowerHeapMutex);

	logDebug("%! initialized with area: %h - %h", "lowerheap", start, end);
	lowerHeapInitialized = true;
}

void* lowerHeapAllocate(uint32_t size)
{
	if(!lowerHeapInitialized)
		panic("%! tried to use uninitialized kernel heap", "lowerheap");

	mutexAcquire(&lowerHeapMutex);

	void* ptr = memoryAllocatorAllocate(&lowerHeapAllocator, size);
	if(!ptr)
		panic("%! failed to allocate lower memory of size %x", "lowerheap", size);

	lowerHeapAmountInUse += size;

	mutexRelease(&lowerHeapMutex);
	return ptr;
}

void lowerHeapFree(void* ptr)
{
	if(!lowerHeapInitialized)
		panic("%! tried to use uninitialized lower heap", "lowerheap");

	mutexAcquire(&lowerHeapMutex);
	lowerHeapAmountInUse -= memoryAllocatorFree(&lowerHeapAllocator, ptr);
	mutexRelease(&lowerHeapMutex);
}

uint32_t lowerHeapGetUsedAmount()
{
	return lowerHeapAmountInUse;
}
