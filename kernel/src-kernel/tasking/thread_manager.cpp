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

#include "tasking/thread_manager.hpp"

#include "kernel.hpp"
#include "ghost/kernel.h"
#include "logger/logger.hpp"
#include "memory/memory.hpp"
#include "memory/address_space.hpp"
#include "memory/gdt/gdt_macros.hpp"
#include "memory/collections/address_stack.hpp"
#include "memory/physical/pp_allocator.hpp"
#include "memory/physical/pp_reference_tracker.hpp"
#include "memory/temporary_paging_util.hpp"
#include "memory/collections/address_range_pool.hpp"
#include "memory/constants.hpp"
#include "memory/lower_heap.hpp"
#include "system/interrupts/descriptors/ivt.hpp"
#include "utils/string.hpp"
#include "tasking/tasking.hpp"
#include "tasking/process.hpp"
#include "filesystem/filesystem.hpp"
#include "tasking/communication/message_controller.hpp"

/**
 *
 */
g_page_directory g_thread_manager::initializePageDirectoryForProcess() {

	g_page_directory currentPd = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;

	// allocate a page for the directory
	g_physical_address physPd = g_pp_allocator::allocate();

	// temporarily map it
	g_page_directory tempPd = (g_page_directory) g_temporary_paging_util::map(physPd);

	// clone mappings for all tables that are not in user range
	for (uint32_t ti = 0; ti < 1024; ti++) {

		if (!((currentPd[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE)) {
			tempPd[ti] = currentPd[ti];
		} else {
			tempPd[ti] = 0;
		}
	}

	// clone mappings for the lowest 4 MiB
	tempPd[0] = currentPd[0];

	// recursively map to self
	tempPd[1023] = physPd | DEFAULT_KERNEL_TABLE_FLAGS;

	// remove the temporary mapping
	g_temporary_paging_util::unmap((g_virtual_address) tempPd);

	return (g_page_directory) physPd;
}

/**
 *
 */
g_physical_address g_thread_manager::forkCurrentPageDirectory(g_process* process, g_thread* sourceThread, g_virtual_address* outKernelStackVirt,
		g_virtual_address* outUserStackVirt) {

	g_page_directory currentPd = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;

	// create the directory
	g_physical_address physPd = g_pp_allocator::allocate();

	// temporary map directory
	g_page_directory tempPd = (g_page_directory) g_temporary_paging_util::map(physPd);

	// deep-copy
	for (uint32_t ti = 1; ti < 1023; ti++) {
		if (currentPd[ti]) {

			// copy kernel tables
			if (ti * 1024 * G_PAGE_SIZE >= G_CONST_KERNEL_AREA_START) {
				tempPd[ti] = currentPd[ti];

				// TODO make virtual ranges stay
			} else if (ti * 1024 * G_PAGE_SIZE >= G_CONST_USER_VIRTUAL_RANGES_START) {
				tempPd[ti] = 0;

				// anything else is deep-copied
			} else {

				// get address of the table
				g_page_table table = G_CONST_RECURSIVE_PAGE_TABLE(ti);
				uint32_t tableFlags = currentPd[ti] & (G_PAGE_ALIGN_MASK);

				// create a new table
				g_physical_address clonedTablePhys = g_pp_allocator::allocate();
				g_page_table clonedTableTemp = (g_page_table) g_temporary_paging_util::map(clonedTablePhys);

				// copy page table entries
				for (uint32_t pi = 0; pi < 1024; pi++) {
					if (table[pi]) {
						// clone page mappings as read-onle
						clonedTableTemp[pi] = table[pi] & ~G_PAGE_TABLE_READWRITE;

						// increment reference count on the physical page
						g_pp_reference_tracker::increment(G_PAGE_ALIGN_DOWN(table[pi]));

					} else {
						clonedTableTemp[pi] = 0;
					}
				}

				g_temporary_paging_util::unmap((g_virtual_address) clonedTableTemp);

				// insert into new page directory
				tempPd[ti] = clonedTablePhys | tableFlags;
			}
		} else {
			tempPd[ti] = 0;
		}
	}

	// clone mappings for the lowest 4 MiB
	tempPd[0] = currentPd[0];

	// recursively map to self
	tempPd[1023] = physPd | DEFAULT_KERNEL_TABLE_FLAGS;

	// clone entire user stack area
	g_virtual_address userStackVirtRange = process->virtualRanges.allocate(G_THREAD_USER_STACK_RESERVED_VIRTUAL_PAGES);
	g_virtual_address userStackStart = userStackVirtRange + (G_THREAD_USER_STACK_RESERVED_VIRTUAL_PAGES - sourceThread->userStackPages) * G_PAGE_SIZE;

	for (uint8_t i = 0; i < sourceThread->userStackPages; i++) {
		g_physical_address userStackPhys = g_pp_allocator::allocate();
		g_virtual_address userStackPageOff = userStackStart + i * G_PAGE_SIZE;
		g_address_space::map_to_temporary_mapped_directory(tempPd, userStackPageOff, userStackPhys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS, true);

		g_virtual_address userStackPageTemp = g_temporary_paging_util::map(userStackPhys);
		g_memory::copy((uint8_t*) userStackPageTemp, (uint8_t*) (sourceThread->userStackAreaStart + i * G_PAGE_SIZE), G_PAGE_SIZE);
		g_temporary_paging_util::unmap(userStackPageTemp);
	}

	// unmap the temporary mapped directory
	g_temporary_paging_util::unmap((g_virtual_address) tempPd);

	// copy kernel stack
	g_virtual_address kernelStackVirt = g_kernel::virtual_range_pool->allocate(1);
	g_physical_address kernelStackPhys = g_pp_allocator::allocate();
	g_address_space::map(kernelStackVirt, kernelStackPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	g_memory::copy((uint8_t*) kernelStackVirt, (uint8_t*) sourceThread->kernelStackPageVirt, G_PAGE_SIZE);

	// modify the forked directory
	for (uint32_t ti = 1; ti < 1023; ti++) {
		if (currentPd[ti]) {
			// process area
			if (ti * 1024 * G_PAGE_SIZE < G_CONST_USER_VIRTUAL_RANGES_START) {
				// make all pages in all tables read-only
				g_page_table table = G_CONST_RECURSIVE_PAGE_TABLE(ti);
				for (uint32_t pi = 0; pi < 1024; pi++) {
					table[pi] = table[pi] & ~G_PAGE_TABLE_READWRITE;
				}

				// virtual ranges
			} else if (ti * 1024 * G_PAGE_SIZE < G_CONST_KERNEL_AREA_START) {
				// TODO what to do with virtual ranges?

			}
		}
	}

	*outKernelStackVirt = kernelStackVirt;
	*outUserStackVirt = userStackStart;

	return physPd;
}

/**
 *
 */
bool g_thread_manager::createThreadUserStack(g_process* process, g_virtual_address* outUserStackVirt) {

	// prepare user stack virtual range and address
	g_virtual_address userStackVirtRange = process->virtualRanges.allocate(G_THREAD_USER_STACK_RESERVED_VIRTUAL_PAGES);
	if (userStackVirtRange == 0) {
		if (process->main) {
			g_log_warn("%! thread creation for process %i failed: no virtual ranges available for stack allocation", "threadmgr", process->main->id);
		} else {
			g_log_warn("%! main thread creation failed: no virtual ranges available for stack allocation", "threadmgr");
		}
		return false;
	}

	// user stack is at the end of the range
	g_virtual_address userStackVirt = userStackVirtRange + (G_THREAD_USER_STACK_RESERVED_VIRTUAL_PAGES - 1) * G_PAGE_SIZE;

	// allocate physical locations
	g_physical_address userStackPhys = g_pp_allocator::allocate();

	// map directory temporary and map user stack
	g_page_directory tempPd = (g_page_directory) g_temporary_paging_util::map((g_physical_address) process->pageDirectory);
	g_address_space::map_to_temporary_mapped_directory(tempPd, userStackVirt, userStackPhys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
	g_temporary_paging_util::unmap((g_virtual_address) tempPd);

	// set out parameters
	*outUserStackVirt = userStackVirt;
	return true;
}

/**
 *
 */
bool g_thread_manager::createThreadKernelStack(g_process* process, g_virtual_address* outKernelStackVirt) {

	// perform stack mapping
	g_virtual_address kernelStackVirt = g_kernel::virtual_range_pool->allocate(1);
	if (kernelStackVirt == 0) {
		if (process->main) {
			g_log_warn("%! thread creation for process %i failed: kernel virtual ranges are full", "threadmgr", process->main->id);
		} else {
			g_log_warn("%! main thread creation failed: kernel virtual ranges are full", "threadmgr");
		}
		return false;
	}

	// allocate physical locations
	g_physical_address kernelStackPhys = g_pp_allocator::allocate();

	// map kernel stack (global space)
	g_address_space::map(kernelStackVirt, kernelStackPhys, DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);

	// set out parameters
	*outKernelStackVirt = kernelStackVirt;
	return true;
}

/**
 *
 */
void g_thread_manager::applySecurityLevel(g_processor_state* state, g_security_level securityLevel) {

	if (securityLevel == G_SECURITY_LEVEL_KERNEL) {
		state->cs = G_GDT_DESCRIPTOR_KERNEL_CODE | G_SEGMENT_SELECTOR_RING0;
		state->ss = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->ds = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->es = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->fs = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
		state->gs = G_GDT_DESCRIPTOR_KERNEL_DATA | G_SEGMENT_SELECTOR_RING0;
	} else {
		state->cs = G_GDT_DESCRIPTOR_USER_CODE | G_SEGMENT_SELECTOR_RING3;
		state->ss = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->ds = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->es = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->fs = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
		state->gs = G_GDT_DESCRIPTOR_USER_DATA | G_SEGMENT_SELECTOR_RING3;
	}

	if (securityLevel <= G_SECURITY_LEVEL_DRIVER) {
		state->eflags |= 0x3000; // IOPL 3
	}
}

/**
 *
 */
g_thread* g_thread_manager::fork(g_thread* source_thread) {

	g_process* parent = source_thread->process;

	g_process* process = new g_process(parent->securityLevel);
	process->virtualRanges.initialize(G_CONST_USER_VIRTUAL_RANGES_START, G_CONST_KERNEL_AREA_START); // TODO clone virtual ranges during forking

	g_virtual_address kernelStackVirt;
	g_virtual_address userStackVirt;
	process->pageDirectory = (g_page_directory) forkCurrentPageDirectory(process, source_thread, &kernelStackVirt, &userStackVirt);

	// copy heap information
	process->heapBreak = parent->heapBreak;
	process->heapPages = parent->heapPages;
	process->heapStart = parent->heapStart;
	process->imageEnd = parent->imageEnd;
	process->imageStart = parent->imageStart;

	// create main thread
	g_virtual_address esp0 = kernelStackVirt + G_PAGE_SIZE;

	g_thread* thread = new g_thread(G_THREAD_TYPE_MAIN);
	thread->cpuState = (g_processor_state*) (esp0 - sizeof(g_processor_state));
	thread->kernelStackEsp0 = esp0;

	thread->kernelStackPageVirt = kernelStackVirt;
	thread->userStackAreaStart = userStackVirt;
	thread->userStackPages = source_thread->userStackPages;

	// link thread to process
	process->parent = parent;
	process->main = thread;
	thread->process = process;

#if G_LOGGING_DEBUG
	dumpTask(thread);
#endif
	return thread;
}

/**
 *
 */
g_thread* g_thread_manager::createProcess(g_security_level securityLevel, g_process* parent) {

	// create the process
	g_process* process = new g_process(securityLevel);
	process->pageDirectory = initializePageDirectoryForProcess();
	process->virtualRanges.initialize(G_CONST_USER_VIRTUAL_RANGES_START, G_CONST_KERNEL_AREA_START);

	// create main thread
	g_thread* main_thread = createThread(process, G_THREAD_TYPE_MAIN);
	process->parent = parent;
	process->main = main_thread;

#if G_LOGGING_DEBUG
	dumpTask(main_thread);
#endif
	return main_thread;
}

/**
 *
 */
g_thread* g_thread_manager::createSubThread(g_process* process) {

	g_thread* thread = createThread(process, G_THREAD_TYPE_SUB);

#if G_LOGGING_DEBUG
	dumpTask(thread);
#endif

	return thread;
}

/**
 *
 */
g_thread* g_thread_manager::createThread(g_process* process, g_thread_type type) {

	// create the stacks
	g_virtual_address kernelStackPageVirt;
	if (!createThreadKernelStack(process, &kernelStackPageVirt)) {
		return nullptr;
	}

	g_virtual_address userStackAreaStart;
	if (!createThreadUserStack(process, &userStackAreaStart)) {
		return nullptr;
	}

	// calculate stack locations
	g_virtual_address esp0 = kernelStackPageVirt + G_PAGE_SIZE;
	g_virtual_address esp = userStackAreaStart + G_PAGE_SIZE;

	// create initial state on the kernel stack
	g_processor_state* state = (g_processor_state*) (esp0 - sizeof(g_processor_state));
	g_memory::setBytes(state, 0, sizeof(g_processor_state));
	state->esp = esp;
	state->eip = 0;
	state->eflags = 0x200;

	// apply security level configuration
	applySecurityLevel(state, process->securityLevel);

	// create the thread
	g_thread* thread = new g_thread(type);
	thread->cpuState = state;
	thread->kernelStackEsp0 = esp0;

	thread->kernelStackPageVirt = kernelStackPageVirt;
	thread->userStackAreaStart = userStackAreaStart;
	thread->userStackPages = 1;

	// link thread to process
	thread->process = process;

	// initialize thread local storage for subthreads
	if (type == G_THREAD_TYPE_SUB) {
		prepareThreadLocalStorage(thread);
	}

	return thread;
}

/**
 *
 */
g_thread* g_thread_manager::createProcessVm86(uint8_t interrupt, g_vm86_registers& in, g_vm86_registers* out) {

	g_process* process = new g_process(G_SECURITY_LEVEL_KERNEL);
	process->pageDirectory = initializePageDirectoryForProcess();

	// create kernel stack
	g_virtual_address kernelStackVirt;
	if (!createThreadKernelStack(process, &kernelStackVirt)) {
		return nullptr;
	}

	// allocate user stack in lower memory
	g_virtual_address userStackVirt = (uint32_t) g_lower_heap::allocate(0x2000);

	// initialize the state
	g_virtual_address esp0 = kernelStackVirt + G_PAGE_SIZE;

	g_processor_state_vm86* state = (g_processor_state_vm86*) (esp0 - sizeof(g_processor_state_vm86));
	g_memory::setBytes(state, 0, sizeof(g_processor_state_vm86));
	state->defaultFrame.eax = in.ax;
	state->defaultFrame.ebx = in.bx;
	state->defaultFrame.ecx = in.cx;
	state->defaultFrame.edx = in.dx;
	state->defaultFrame.ebp = 0;
	state->defaultFrame.esi = in.si;
	state->defaultFrame.edi = in.di;

	state->defaultFrame.eip = G_FP_OFF(ivt->entry[interrupt]);
	state->defaultFrame.cs = G_FP_SEG(ivt->entry[interrupt]);
	state->defaultFrame.eflags = 0x20202;
	state->defaultFrame.esp = 0x1000;
	state->defaultFrame.ss = (((userStackVirt & ~(0xFFF)) + 0x1000) >> 4);

	state->gs = 0x00;
	state->fs = 0x00;
	state->es = in.es;
	state->ds = in.ds;

	// create main thread
	g_thread* thread = new g_thread(G_THREAD_TYPE_VM86);
	thread->cpuState = (g_processor_state*) state;
	thread->kernelStackEsp0 = esp0;

	thread->kernelStackPageVirt = kernelStackVirt;
	thread->userStackAreaStart = userStackVirt;
	thread->userStackPages = 1;

	thread->getVm86Information()->out = out;

	// assign thread to process
	process->main = thread;
	thread->process = process;

#if G_LOGGING_DEBUG
	dumpTask(thread);
#endif

	return thread;
}

/**
 *
 */
void g_thread_manager::prepareThreadLocalStorage(g_thread* thread) {

	// if tls master copy available, copy it to thread
	g_process* process = thread->process;
	if (process->tls_master_in_proc_location) {

		// calculate size that TLS needs including alignment
		uint32_t tls_master_aligned_total_size = G_ALIGN_UP(process->tls_master_totalsize, process->tls_master_alignment);

		// allocate virtual range with aligned size of TLS + size of {g_user_thread}
		uint32_t required_size = tls_master_aligned_total_size + sizeof(g_user_thread);
		uint32_t required_pages = G_PAGE_ALIGN_UP(required_size) / G_PAGE_SIZE;
		g_virtual_address tls_copy_virt = process->virtualRanges.allocate(required_pages, G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER);

		// store executing space
		g_page_directory current = g_address_space::get_current_space();

		// temporarily switch to target process directory, copy TLS contents
		g_address_space::switch_to_space(process->pageDirectory);
		for (uint32_t i = 0; i < required_pages; i++) {
			g_physical_address phys = g_pp_allocator::allocate();
			g_address_space::map(tls_copy_virt + i * G_PAGE_SIZE, phys,
			DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			g_pp_reference_tracker::increment(phys);
		}

		// zero & copy TLS content
		g_memory::setBytes((void*) tls_copy_virt, 0, process->tls_master_totalsize);
		g_memory::copy((void*) tls_copy_virt, (void*) process->tls_master_in_proc_location, process->tls_master_copysize);

		// fill user thread
		g_virtual_address user_thread_loc = tls_copy_virt + tls_master_aligned_total_size;
		g_user_thread* user_thread = (g_user_thread*) user_thread_loc;
		user_thread->self = user_thread;

		// switch back
		g_address_space::switch_to_space(current);

		// set threads TLS location
		thread->user_thread_addr = user_thread_loc;
		thread->tls_copy_virt = tls_copy_virt;

		g_log_debug("%! created tls copy in process %i, thread %i at %h", "threadmgr", process->main->id, thread->id, thread->tls_copy_virt);
	}

}

/**
 * Deletes a task and cleans up all its memory. This task must not be in the
 * scheduling list anymore.
 *
 * TODO FIXME XXX
 */
void g_thread_manager::deleteTask(g_thread* task) {

	G_DEBUG_INTERFACE_TASK_SET_STATUS(task->id, "dead");

	// clear message queues
	g_message_controller::clear(task->id);

	if (task->type == G_THREAD_TYPE_SUB) {
		g_process* process = task->process;

		// remove kernel stack
		g_pp_allocator::free(g_address_space::virtual_to_physical(task->kernelStackPageVirt));

		// remove user stack
		g_page_directory currentSpace = g_address_space::get_current_space();
		g_address_space::switch_to_space(process->pageDirectory);
		g_pp_allocator::free(g_address_space::virtual_to_physical(task->userStackAreaStart));
		g_address_space::switch_to_space(currentSpace);

		// TODO

	} else if (task->type == G_THREAD_TYPE_MAIN) {

		// Here we free everything that the process has created and that is no more
		// needed by anyone.
		//g_process* process = task->process;

		// tell the filesystem to clean up
		g_filesystem::process_closed(task->id);

		// TODO

	} else if (task->type == G_THREAD_TYPE_VM86) {

		// User stack is in memory < 1MB so don't unmap
		g_lower_heap::free((void*) task->userStackAreaStart);

		// TODO:
		// figure out vm86 deletion
	}

	g_log_debug("%! task %i has died, now %i free phys pages", "threadmgr", task->id, g_pp_allocator::getFreePageCount());
	delete task;
	return;
}

/**
 *
 */
void g_thread_manager::dumpTask(g_thread* task) {
#if G_LOGGING_DEBUG
	g_log_debug("%! created %s %i", "threadmgr",
			(task->type == G_THREAD_TYPE_SUB ? "thread" : (task->type == G_THREAD_TYPE_VM86 ? "vm86 process" : "process")), task->id);

	g_process* process = task->process;
	g_log_debugn("%#  process: ");
	if (task->process) {
		g_log_debug("%i", process->main->id);
	} else {
		g_log_debug("none");
	}

	g_log_debug("%#  pagedir:  %h", process->pageDirectory);
	g_log_debug("%#  security: %h", process->securityLevel);
	g_log_debug("%#  kernel sp:     %h", task->kernelStackEsp0);
	if (task->type == G_THREAD_TYPE_VM86) {
		g_log_debug("%#  cs:ip:  %h:%h", task->cpuState->cs, task->cpuState->eip);
		g_log_debug("%#  ax: %h bx: %h cx: %h dx: %h", task->cpuState->eax, task->cpuState->ebx, task->cpuState->ecx, task->cpuState->edx);g_log_debug(
				"%#  user sp:  %h:%h", task->cpuState->esp, task->cpuState->ss);
	} else {
		g_log_debug("%#  user sp:       %h", task->cpuState->esp);g_log_debug("%#  entry point:   %h", task->cpuState->eip);
	}
#endif
}

/**
 *
 */
g_virtual_address g_thread_manager::getMemoryUsage(g_thread* task) {

	g_virtual_address total = 0;

	total += (task->process->heapPages * G_PAGE_SIZE);

	return total;
}
