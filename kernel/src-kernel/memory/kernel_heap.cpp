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

#include <memory/kernel_heap.hpp>
#include <logger/logger.hpp>
#include <kernel.hpp>
#include <memory/address_space.hpp>
#include <memory/paging.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/constants.hpp>
#include <memory/allocators/chunk_allocator.hpp>

/**
 * new
 */
void* operator new(long unsigned int size) {
	return g_kernel_heap::allocate(size);
}

/**
 * new[]
 */
void* operator new[](long unsigned int size) {
	return g_kernel_heap::allocate(size);
}

/**
 * delete implementation
 */
void operator delete(void* m) {
	g_kernel_heap::free(m);
}

/**
 * delete[] implementation
 */
void operator delete[](void* m) {
	g_kernel_heap::free(m);
}

static g_chunk_allocator allocator;

static g_virtual_address heapStart = 0;
static g_virtual_address heapEnd = 0;

static uint32_t usedMemoryAmount = 0;
static bool kernelHeapInitialized = false;

/**
 *
 */
uint32_t g_kernel_heap::getUsedMemoryAmount() {
	return usedMemoryAmount;
}

/**
 *
 */
void g_kernel_heap::initialize(g_virtual_address start, g_virtual_address end) {

	allocator.initialize(start, end);
	heapStart = start;
	heapEnd = end;

	g_log_debug("%! initialized with area: %h - %h", "kernheap", start, end);
	kernelHeapInitialized = true;
}

/**
 *
 */
bool g_kernel_heap::expandHeap() {
	if (heapEnd > G_CONST_KERNEL_HEAP_MAXIMUM_END) {
		g_log_debug("%! maximum reached during expansion", "kernheap");
		return false;
	}

	// Expand virtual space
	for (g_virtual_address v = heapEnd; v < heapEnd + G_KERNEL_HEAP_EXPAND_STEP; v += G_PAGE_SIZE) {
		g_physical_address p = g_pp_allocator::allocate();

		if (p == 0) {
			g_log_warn("%! no pages left for expanding", "kernheap");
			return false;
		}

		g_address_space::map(v, p, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	}

	// Create header
	allocator.expand(G_KERNEL_HEAP_EXPAND_STEP);
	heapEnd += G_KERNEL_HEAP_EXPAND_STEP;

	g_log_debug("%! expanded to end %h (%ikb in use)", "kernheap", heapEnd, usedMemoryAmount / 1024);
	return true;
}

/**
 *
 */
void* g_kernel_heap::allocate(uint32_t size) {

	if (!kernelHeapInitialized) {
		g_kernel::panic("%! tried to use uninitialized kernel heap", "kernheap");
	}

	void* allocated = allocator.allocate(size);
	if (allocated) {
		usedMemoryAmount += size;
		return allocated;
	} else {
		if (expandHeap()) {
			return allocate(size);
		}
	}

	g_kernel::panic("%! could not expand kernel heap", "kernheap");
	return 0;
}

/**
 *
 */
void g_kernel_heap::free(void* mem) {

	if (!kernelHeapInitialized) {
		g_kernel::panic("%! tried to use uninitialized kernel heap", "kernheap");
	}

	usedMemoryAmount -= allocator.free(mem);
}
