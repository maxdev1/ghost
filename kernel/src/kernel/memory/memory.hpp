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

#ifndef __KERNEL_MEMORY__
#define __KERNEL_MEMORY__

#include <limine.h>
#include <ghost/stdint.h>
#include <stddef.h>

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/address_range_pool.hpp"
#include "kernel/memory/bitmap_page_allocator.hpp"

#define G_ALIGN_UP(value, boundary)    (((value) + ((boundary) - 1)) & ~((boundary) - 1))
#define G_ALIGN_DOWN(value, boundary)  ((value) & ~((boundary) - 1))

/**
 * Atomic operations on integers, booleans and pointers. Order arguments must be
 * compile-time G_MEMORY_ORDER_* constants. Loads allow relaxed/acquire ordering,
 * stores allow relaxed/release ordering, and read-modify-write operations allow
 * all orders below. Compare-exchange updates expected on failure; its failure
 * order must be relaxed/acquire and no stronger than its success order.
 * FetchAdd/FetchSub return the old value; AddFetch/SubFetch return the new value.
 */
#define G_MEMORY_ORDER_RELAXED __ATOMIC_RELAXED
#define G_MEMORY_ORDER_ACQUIRE __ATOMIC_ACQUIRE
#define G_MEMORY_ORDER_RELEASE __ATOMIC_RELEASE
#define G_MEMORY_ORDER_ACQ_REL __ATOMIC_ACQ_REL

#define memoryAtomicLoad(pointer, order) __atomic_load_n((pointer), (order))
#define memoryAtomicStore(pointer, value, order) __atomic_store_n((pointer), (value), (order))
#define memoryAtomicExchange(pointer, value, order) __atomic_exchange_n((pointer), (value), (order))
#define memoryAtomicCompareExchange(pointer, expected, value, weak, successOrder, failureOrder) \
	__atomic_compare_exchange_n((pointer), (expected), (value), (weak), (successOrder), (failureOrder))
#define memoryAtomicAddFetch(pointer, value, order) __atomic_add_fetch((pointer), (value), (order))
#define memoryAtomicSubFetch(pointer, value, order) __atomic_sub_fetch((pointer), (value), (order))
#define memoryAtomicFetchAdd(pointer, value, order) __atomic_fetch_add((pointer), (value), (order))
#define memoryAtomicFetchSub(pointer, value, order) __atomic_fetch_sub((pointer), (value), (order))
#define memoryAtomicFence(order) __atomic_thread_fence((order))

class g_task;
class g_process;

extern g_address_range_pool* memoryVirtualRangePool;

void memoryInitialize(limine_memmap_response* memoryMap);

/**
 * Checks whether a non-empty byte range fits the CPU's physical address width.
 * Does not check page alignment, memory availability, or ownership.
 */
bool memoryPhysicalIsRangeAddressable(g_physical_address address, g_size size);

/**
 * Converts a non-empty byte size to a page count without rounding/narrowing overflow.
 */
bool memorySizeToPageCount(g_size size, uint32_t* outPages);

/**
 * Allocates a physical memory page.
 */
g_physical_address memoryPhysicalAllocate(bool untracked = false);
g_physical_address memoryPhysicalTryAllocate();

/**
 * Frees a physical memory page.
 */
void memoryPhysicalFree(g_physical_address page);

/**
 * Allocates and maps a memory range with the given number of pages.
 */
g_virtual_address memoryAllocateKernel(int32_t pages);

/**
 * Frees a memory range allocated with <memoryAllocateKernelRange>.
 */
void memoryFreeKernelRange(g_virtual_address address);

/**
 * Creates an on-demand mapping for a file in memory.
 */
void memoryOnDemandMapFile(g_process* process, g_fd file, g_offset fileOffset, g_address fileStart, g_ptrsize fileSize,
                           g_ptrsize memorySize);

/**
 * Searches for an on-demand mapping containing the given address.
 */
g_memory_file_ondemand* memoryOnDemandFindMapping(g_task* task, g_address address);

/**
 * Handles loading of the on-demand mapped file content.
 */
bool memoryOnDemandHandlePageFault(g_task* task, g_address accessed);

/**
 * Reference to the loaders or kernels physical page allocator.
 */
extern g_bitmap_page_allocator memoryPhysicalAllocator;

/**
 * Sets number bytes at target to value.
 *
 * @param target	the target pointer
 * @param value		the byte value
 * @param number	the number of bytes to set
 */
void* memorySetBytes(void* target, uint8_t value, int32_t number);

/**
 * Sets number words at target to value.
 *
 * @param target	the target pointer
 * @param value		the word value
 * @param number	the number of word to set
 */
void* memorySetWords(void* target, uint16_t value, int32_t number);

/**
 * Copys size bytes from source to target.
 *
 * @param source	pointer to the source memory location
 * @param target	pointer to the target memory location
 * @param size		number of bytes to copy
 */
void* memoryCopy(void* target, const void* source, int32_t size);

/**
 * Copies from managed user pages into kernel memory. The task must own the active
 * address space. Demand pages are resolved without holding the process lock.
 * Returns false on invalid memory; preceding bytes may already have been copied.
 */
bool memoryCopyFromUser(g_task* task, void* target, g_address source, size_t length);

/**
 * Copies kernel bytes to writable managed user pages, with the same address-space
 * and partial-copy contract as memoryCopyFromUser.
 */
bool memoryCopyToUser(g_task* task, g_address target, const void* source, size_t length);

#endif
