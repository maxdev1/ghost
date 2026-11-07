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

#ifndef GHOST_API_MEMORY
#define GHOST_API_MEMORY

#include "common.h"
#include "stdint.h"
#include "memory/types.h"
#include "tasks/types.h"

/**
 * Atomic operations on integers, booleans and pointers. Order arguments must be
 * compile-time G_MEMORY_ORDER_* constants. Loads allow relaxed/acquire/seq-cst
 * ordering, stores allow relaxed/release/seq-cst ordering, and read-modify-write
 * operations allow all orders below. Compare-exchange updates expected on
 * failure; its failure order must be relaxed/acquire/seq-cst and no stronger
 * than its success order. SignalFence orders compiler accesses only.
 * FetchAdd/FetchSub return the old value; AddFetch/SubFetch return the new value.
 */
#define G_MEMORY_ORDER_RELAXED __ATOMIC_RELAXED
#define G_MEMORY_ORDER_ACQUIRE __ATOMIC_ACQUIRE
#define G_MEMORY_ORDER_RELEASE __ATOMIC_RELEASE
#define G_MEMORY_ORDER_ACQ_REL __ATOMIC_ACQ_REL
#define G_MEMORY_ORDER_SEQ_CST __ATOMIC_SEQ_CST

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
#define memoryAtomicSignalFence(order) __atomic_signal_fence((order))

__BEGIN_C

/**
 * Allocates a memory region with the size of at least the given
 * size in bytes. This region can for example be used as shared memory.
 *
 * Allocating memory using this call makes the requesting process the physical owner of the
 * pages in its virtual space (important for unmapping).
 *
 * @param size
 * 		the size in bytes
 *
 * @param out_phys
 *		optionally, the physical output, only for security-level DRIVER and only
 *		if just one page was allocated
 *
 * @return a pointer to the allocated memory region, or 0 if failed
 *
 * @security-level APPLICATION
 */
void* g_alloc_mem(g_size size);
void* g_alloc_mem_p(g_size size, void** out_phys);

/**
 * Shares a memory area with another process.
 *
 * @param memory
 * 		a pointer to the memory area to share
 * @param size
 * 		the size of the memory area
 * @param pid
 * 		the id of the target process
 *
 * @return a pointer to the shared memory location within the target address space
 *
 * @security-level APPLICATION
 */
void* g_share_mem(void* memory, int32_t size, g_pid pid);

/**
 * Maps the given physical address to the executing processes address space so
 * it can access it directly.
 *
 * @param addr
 * 		the physical memory address that should be mapped
 * @param size
 * 		the size that should be mapped
 *
 * @return a pointer to the mapped area within the executing processes address space
 *
 * @security-level DRIVER
 */
void* g_map_mmio(void* addr, uint32_t size);

/**
 * Unmaps the given memory area.
 *
 * @param area
 * 		a pointer to the area
 *
 * @security-level DRIVER
 */
void g_unmap(void* area);

/**
 * Frees a memory area allocated with {g_lower_malloc}.
 *
 * @param memory
 * 		a pointer to the memory area
 *
 * @security-level DRIVER
 */
void g_lower_free(void* memory);

/**
 * Allocates a memory area in the lower 1MiB. This can be used
 * for example for the virtual 8086 mode.
 *
 * @param size
 * 		the size to allocate
 *
 * @security-level DRIVER
 */
void* g_lower_malloc(uint32_t size);

/**
 * Adjusts the program heap break.
 *
 * @param amount
 * 		the value to adjust the break by
 * @param out_brk	is filled with the result
 *
 * @return whether adjusting was successful
 *
 * @security-level APPLICATION
 */
uint8_t g_sbrk(int amount, void** out_brk);


__END_C

#endif
