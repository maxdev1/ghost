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
 * Allocates zero-filled, page-aligned memory in the calling process.
 *
 * Equivalent to g_alloc_mem_p(size, 0). This is a separate mapping, not a malloc() heap
 * allocation.
 *
 * @param[in] size Positive minimum size in bytes, rounded up to whole pages.
 * @return The allocation's base address, or null on failure. Release it with g_unmap().
 * @security-level APPLICATION
 */
void* g_alloc_mem(g_size size);

/**
 * Allocates zero-filled pages and optionally reports a physical address.
 *
 * @param[in] size Positive minimum size in bytes, rounded up to whole pages.
 * @param[out] out_phys Receives the physical address only for a single-page allocation by a DRIVER
 *                      or KERNEL caller; otherwise receives null. May be null.
 * @return The page-aligned virtual base, or null on failure. Release it with g_unmap().
 * @security-level APPLICATION
 */
void* g_alloc_mem_p(g_size size, void** out_phys);

/**
 * Maps pages from the calling process into another process.
 *
 * Both mappings access the same writable pages. Each process releases its own mapping with
 * g_unmap(); managed pages remain allocated until their final mapping is released.
 *
 * @param[in] memory Page-aligned base of mapped source memory.
 * @param[in] size Positive number of bytes to share, rounded up to whole pages.
 * @param[in] pid ID of the target process.
 * @return The mapping's base in the target process, or null on failure; it is not a local pointer.
 * @security-level APPLICATION
 */
void* g_share_mem(void* memory, int32_t size, g_pid pid);

/**
 * Maps a physical memory region into the calling process.
 *
 * Release the mapping with g_unmap(). Unmapping does not free the underlying physical device
 * memory.
 *
 * @param[in] addr Page-aligned physical base address, expressed as a pointer.
 * @param[in] size Positive minimum mapping size in bytes, rounded up to whole pages.
 * @return The page-aligned virtual base, or null on failure or insufficient privilege.
 * @security-level DRIVER
 */
void* g_map_mmio(void* addr, uint32_t size);

/**
 * Releases an entire separately allocated or shared virtual mapping.
 *
 * The mapping is removed from the calling process. Managed pages are freed when no mappings retain
 * them; device pages are not freed. This does not accept malloc() blocks or interior pointers.
 *
 * @param[in] area Exact page-aligned mapping base returned by g_alloc_mem(), g_share_mem() in this
 *                 process, or g_map_mmio().
 * @security-level APPLICATION
 */
void g_unmap(void* area);

/**
 * Releases an allocation from the lower-memory allocator.
 *
 * @param[in] memory Allocation returned by g_lower_malloc(); must not already be freed.
 * @security-level DRIVER
 */
void g_lower_free(void* memory);

/**
 * Allocates memory from the legacy lower 1 MiB region.
 *
 * The current allocator panics on exhaustion rather than returning a recoverable allocation
 * failure.
 *
 * @param[in] size Number of bytes to allocate.
 * @return The allocation to release with g_lower_free(), or null for an unprivileged caller.
 * @security-level DRIVER
 */
void* g_lower_malloc(uint32_t size);

/**
 * Adjusts the calling process's heap break by a signed byte count.
 *
 * Coordinate direct use with the process allocator, which may also manage this heap.
 *
 * @param[in] amount Bytes to add or remove; zero queries the current break.
 * @param[out] out_brk Non-null output receiving the previous break on success, or the all-bits-set
 *                     address on failure.
 * @return Nonzero on success, or zero on failure; failure leaves the heap unchanged.
 * @security-level APPLICATION
 */
uint8_t g_sbrk(int amount, void** out_brk);


__END_C

#endif
