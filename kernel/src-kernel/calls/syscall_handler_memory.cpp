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

#include <calls/syscall_handler.hpp>

#include <kernel.hpp>
#include <logger/logger.hpp>
#include <executable/elf32_loader.hpp>
#include <tasking/tasking.hpp>
#include <tasking/thread_manager.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/physical/pp_reference_tracker.hpp>
#include <memory/address_space.hpp>
#include <memory/temporary_paging_util.hpp>
#include <memory/constants.hpp>
#include <memory/lower_heap.hpp>

/**
 * Allocates a memory area of at least "size" bytes. Memory is always allocated page-wise,
 * therefore the area is always page-aligned and has a size of a multiple of the page size.
 *
 * Allocating memory using this call makes the requesting process the physical owner of the
 * pages in its virtual space (important for unmapping).
 */
G_SYSCALL_HANDLER(alloc_mem) {

	g_process* process = current_thread->process;

	g_syscall_alloc_mem* data = (g_syscall_alloc_mem*) G_SYSCALL_DATA(current_thread->cpuState);
	data->virtualResult = 0;

	// Get the number of pages
	uint32_t pages = G_PAGE_ALIGN_UP(data->size) / G_PAGE_SIZE;
	if (pages > 0) {
		// Allocate a virtual range, we are physical owner
		uint8_t virtualRangeFlags = G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER;
		g_virtual_address virtualRangeBase = process->virtualRanges.allocate(pages, virtualRangeFlags);

		if (virtualRangeBase != 0) {
			bool mappingSuccessful = true;

			// Perform either page-wise mapping
			for (uint32_t i = 0; i < pages; i++) {
				g_physical_address addr = g_pp_allocator::allocate();

				if (addr == 0) {
					mappingSuccessful = false;
					break;
				}

				g_address_space::map(virtualRangeBase + i * G_PAGE_SIZE, addr,
				DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			}

			// Check if the mapping was successful
			if (mappingSuccessful) {
				// Successful, return the range base
				data->virtualResult = (void*) virtualRangeBase;

				g_log_debug("%! allocated memory area of size %h at virt %h for process %i", "syscall", pages * G_PAGE_SIZE, data->virtualResult,
						process->main->id);
			} else {

				// Failed to create the area, free everything
				g_log_warn("%! went out of memory during shared memory allocation for process %i, rollback...", "syscall", process->main->id);
				for (uint32_t i = 0; i < pages; i++) {
					g_physical_address addr = g_address_space::virtual_to_physical(virtualRangeBase + i * G_PAGE_SIZE);

					if (addr) {
						g_pp_allocator::free(addr);
					}
				}
				process->virtualRanges.free(virtualRangeBase);
			}
		}
	}

	return current_thread;
}

/**
 * Shares the memory area at "memory" with a size of "size" with the process that has the
 * identifier "processId". The "virtualAddress" contains the virtual address of the area
 * in the target processes address space if the sharing was successful. Memory is always
 * shared page-wise. The resulting virtual address in the target space is always
 * page-aligned.
 *
 * The area to share should always is always page-wise and the given address must be
 * page-aligned, thus only memory that was allocated page-wise should be shared.
 *
 * The target process then is not the physical owner of the shared pages.
 */
G_SYSCALL_HANDLER(share_mem) {

	G_IF_LOG_WARN(g_process* process = current_thread->process);

	g_syscall_share_mem* data = (g_syscall_share_mem*) G_SYSCALL_DATA(current_thread->cpuState);
	data->virtualAddress = 0;

	// Get the number of pages
	uint32_t memory = (uint32_t) data->memory;
	uint32_t pages = G_PAGE_ALIGN_UP(data->size) / G_PAGE_SIZE;

	// Only allow sharing in user memory
	if (memory < G_CONST_KERNEL_AREA_START && (memory + pages * G_PAGE_SIZE) <= G_CONST_KERNEL_AREA_START) {

		// Get the target process
		g_thread* targetThread = g_tasking::getTaskById(data->processId);
		if (targetThread) {
			g_process* targetProcess = targetThread->process;

			// Get the range in the other process space
			g_virtual_address virtualRangeBase = targetProcess->virtualRanges.allocate(pages,
			G_PROC_VIRTUAL_RANGE_FLAG_NONE);

			if (virtualRangeBase != 0) {

				g_page_directory executing_space = g_address_space::get_current_space();

				// Map the pages to the other processes space
				for (uint32_t i = 0; i < pages; i++) {
					g_physical_address physical_addr = g_address_space::virtual_to_physical(memory + i * G_PAGE_SIZE);

					g_address_space::switch_to_space(targetProcess->pageDirectory);
					g_address_space::map(virtualRangeBase + i * G_PAGE_SIZE, physical_addr, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
					g_address_space::switch_to_space(executing_space);
				}

				// Done
				data->virtualAddress = (void*) virtualRangeBase;

				g_log_debug("%! shared memory area of process %i at %h of size %h with process %i to address %h", "syscall", process->main->id, memory,
						pages * G_PAGE_SIZE, targetProcess->main->id, virtualRangeBase);
			} else {
				g_log_warn("%! process %i was unable to share memory are %h of size %h with process %i because there was no virtual range", "syscall",
						process->main->id, memory, pages * G_PAGE_SIZE, targetProcess->main->id);
			}
		} else {
			g_log_warn("%! process %i was unable to share memory with the other process because it was null", "syscall", process->main->id);
		}
	} else {
		g_log_warn("%! process %i was unable to share memory because addresses below %h are not allowed", "syscall", process->main->id,
				G_CONST_KERNEL_AREA_START);
	}

	return current_thread;

}

/**
 * Maps the physical area at "physicalAddress" with a size of at least "size" to the
 * given "virtualAddress" in the current address space. Mapping is always done page-wise.
 *
 * This does not make the current process the physical owner of the physical pages,
 * as they may possibly not be used otherwise once the process unmaps them.
 */
G_SYSCALL_HANDLER(map_mmio) {

	g_process* process = current_thread->process;

	g_syscall_map_mmio* data = (g_syscall_map_mmio*) G_SYSCALL_DATA(current_thread->cpuState);
	data->virtualAddress = 0;

	// Only kernel and drivers may do this
	if (process->securityLevel <= G_SECURITY_LEVEL_DRIVER) {

		uint32_t pages = G_PAGE_ALIGN_UP(data->size) / G_PAGE_SIZE;

		// Allocate a virtual range (but not be physical owner)
		g_virtual_address range = process->virtualRanges.allocate(pages,
		G_PROC_VIRTUAL_RANGE_FLAG_NONE);

		uint32_t physical = (uint32_t) data->physicalAddress;

		if (pages > 0 && range != 0) {
			// Map the pages to the space
			for (uint32_t i = 0; i < pages; i++) {

				g_address_space::map(range + i * G_PAGE_SIZE, physical + i * G_PAGE_SIZE,
				DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			}

			// Logger_infoln("%! mapped MMIO area %h to %h to virtual %h of process %i", "syscall", physical, physical + pages * PAGE_SIZE, range, process->main->id);

			data->virtualAddress = (void*) range;
		}
	}

	return current_thread;
}

/**
 * Unmaps the area at the given "virtualBase". This address must be page-aligned.
 *
 * If the current process is the physical owner of the pages within this area,
 * these physical pages are freed.
 */
G_SYSCALL_HANDLER(unmap) {

	g_process* process = current_thread->process;

	g_syscall_unmap* data = (g_syscall_unmap*) G_SYSCALL_DATA(current_thread->cpuState);
	g_virtual_address base = data->virtualBase;

	// Search for the range
	g_address_range* range = process->virtualRanges.getRanges();
	while (range) {
		if (range->base == base) {
			break;
		}

		range = range->next;
	}

	// Found range, free it
	if (range) {

		// If physical owner, free physical range
		if (range->flags & G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER) {
			// free each page
			for (uint32_t i = 0; i < range->pages; i++) {
				g_physical_address physicalPage = g_address_space::virtual_to_physical(range->base + i * G_PAGE_SIZE);
				g_pp_allocator::free(physicalPage);
			}
		}

		// Unmap pages
		for (uint32_t i = 0; i < range->pages; i++) {
			g_address_space::unmap(range->base + i * G_PAGE_SIZE);
		}

		// Free the virtual range
		process->virtualRanges.free(range->base);
		g_log_debug("%! task %i in process %i unmapped range at %h", "syscall", process->main->id, current_thread->id, base);
	} else {
		g_log_warn("%! task %i in process %i tried to unmap range at %h that was never mapped", "syscall", process->main->id, current_thread->id, base);
	}

	return current_thread;
}

/**
 * Increases/decreases the program heap break of the current process.
 */
G_SYSCALL_HANDLER(sbrk) {

	g_process* process = current_thread->process;
	g_syscall_sbrk* data = (g_syscall_sbrk*) G_SYSCALL_DATA(current_thread->cpuState);

	// initialize the heap if necessary
	if (process->heapBreak == 0) {
		g_virtual_address heapStart = process->imageEnd;

		g_physical_address phys = g_pp_allocator::allocate();
		g_address_space::map(heapStart, phys, DEFAULT_USER_TABLE_FLAGS,
		DEFAULT_USER_PAGE_FLAGS);
		g_pp_reference_tracker::increment(phys);

		process->heapBreak = heapStart;
		process->heapStart = heapStart;
		process->heapPages = 1;
		g_log_debug("%! process %i initializes his heap at %h", "syscall", process->main->id, heapStart);
	}

	// calculate new address
	g_virtual_address brk_old = process->heapBreak;
	g_virtual_address brk_new = brk_old + data->amount;

	// heap expansion is limited
	if (brk_new >= G_CONST_USER_MAXIMUM_HEAP_BREAK) {
		g_log_info("%! process %i went out of memory when setting heap break", "syscall", process->main->id);
		data->address = -1;
		data->successful = false;

	} else {
		// expand if necessary
		g_virtual_address virt_above;
		while (brk_new > (virt_above = process->heapStart + process->heapPages * G_PAGE_SIZE)) {
			g_physical_address phys = g_pp_allocator::allocate();
			g_address_space::map(virt_above, phys, DEFAULT_USER_TABLE_FLAGS,
			DEFAULT_USER_PAGE_FLAGS);
			g_pp_reference_tracker::increment(phys);
			++process->heapPages;
		}

		// shrink if possible
		g_virtual_address virt_below;
		while (brk_new < (virt_below = process->heapStart + process->heapPages * G_PAGE_SIZE - G_PAGE_SIZE)) {
			g_physical_address phys = g_address_space::virtual_to_physical(virt_below);
			g_address_space::unmap(virt_below);
			if (g_pp_reference_tracker::decrement(phys) == 0) {
				g_pp_allocator::free(phys);
			}
			--process->heapPages;
		}

		process->heapBreak = brk_new;
		data->address = brk_old;
		data->successful = true;
	}

	g_log_debug("%! <%i> sbrk(%i): %h -> %h (%h -> %h, %i pages)", "syscall", process->main->id, data->amount, data->address, process->heapBreak,
			process->heapStart, process->heapStart + process->heapPages * G_PAGE_SIZE, process->heapPages);

	return current_thread;
}

/**
 * Allocates a memory chunk in the area below 1MiB. These memory areas can then
 * be used for calling interrupts using the virtual 8086 mode.
 */
G_SYSCALL_HANDLER(lower_malloc) {

	g_process* process = current_thread->process;
	g_syscall_lower_malloc* data = (g_syscall_lower_malloc*) G_SYSCALL_DATA(current_thread->cpuState);

	// check the security
	if (process->securityLevel <= G_SECURITY_LEVEL_DRIVER) {
		data->result = g_lower_heap::allocate(data->size);
	} else {
		g_log_debug("%! task %i tried to allocate lower memory but does not have permission", "syscall", current_thread->id);
		data->result = 0;
	}

	return current_thread;
}

/**
 * Frees a memory chunk that was allocated using the lower memory allocation call.
 */
G_SYSCALL_HANDLER(lower_free) {

	g_process* process = current_thread->process;
	g_syscall_lower_free* data = (g_syscall_lower_free*) G_SYSCALL_DATA(current_thread->cpuState);

	if (process->securityLevel <= G_SECURITY_LEVEL_DRIVER) {
		g_lower_heap::free(data->memory);
	} else {
		g_log_debug("%! task %i tried to free lower memory but does not have permission", "syscall", current_thread->id);
	}
	return current_thread;
}

