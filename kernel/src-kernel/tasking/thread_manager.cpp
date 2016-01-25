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
g_physical_address g_thread_manager::prepareSpaceForProcess(g_virtual_address kernelStack, g_virtual_address userStack) {

	// Setup the page directory
	g_physical_address newDirPhys = g_pp_allocator::allocate();

	// XXX TEMPORARY MAPPING XXX
	{
		g_virtual_address tempDirectoryAddress = g_temporary_paging_util::map(newDirPhys);

		g_page_directory tempDirectory = (g_page_directory) tempDirectoryAddress;
		g_page_directory currentDirectory = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;

		// Copy all tables without user-flag
		for (uint32_t ti = 0; ti < 1024; ti++) {

			if (!((currentDirectory[ti] & G_PAGE_ALIGN_MASK) & G_PAGE_TABLE_USERSPACE)) {
				tempDirectory[ti] = currentDirectory[ti];
			} else {
				tempDirectory[ti] = 0;
			}
		}

		tempDirectory[0] = currentDirectory[0]; // lowest 4 MiB
		tempDirectory[1023] = newDirPhys | DEFAULT_KERNEL_TABLE_FLAGS; // recursive-ness

		// User stack is not mandatory here
		if (userStack != 0) {
			g_physical_address userStackPhys = g_pp_allocator::allocate();
			g_address_space::map_to_temporary_mapped_directory(tempDirectory, userStack, userStackPhys, DEFAULT_USER_TABLE_FLAGS,
			DEFAULT_USER_PAGE_FLAGS);
		}

		g_temporary_paging_util::unmap(tempDirectoryAddress);
	}
	// XXX

	// Map kernel stack
	g_physical_address kernelStackPhys = g_pp_allocator::allocate();
	g_address_space::map(kernelStack, kernelStackPhys,
	DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);

	return newDirPhys;
}

/**
 *
 */
g_physical_address g_thread_manager::prepareSpaceForFork(g_thread* current, g_virtual_address newKernelStackVirt, g_virtual_address newUserStackVirt) {

	g_page_directory curDir = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;

	// Setup the page directory
	g_physical_address newDirPhys = g_pp_allocator::allocate();

	// XXX TEMPORARY MAPPING XXX
	{
		g_virtual_address newDirTempAddr = g_temporary_paging_util::map(newDirPhys);

		g_page_directory newDir = (g_page_directory) newDirTempAddr;

		// Copy all tables and pages
		for (uint32_t ti = 1; ti < 1023; ti++) {
			if (curDir[ti]) {

				if (ti * 1024 * G_PAGE_SIZE >= G_CONST_KERNEL_AREA_START) { // Kernel tables
					newDir[ti] = curDir[ti];

				} else if (ti * 1024 * G_PAGE_SIZE >= G_CONST_USER_VIRTUAL_RANGES_START) { // Virtual ranges
					// TODO: make virtual range tables stay
					newDir[ti] = 0;

				} else {
					g_page_table curTbl = G_CONST_RECURSIVE_PAGE_TABLE(ti);
					uint32_t curTblFlags = curDir[ti] & (G_PAGE_ALIGN_MASK);

					// create a new table
					g_physical_address newTblPhys = g_pp_allocator::allocate();
					g_virtual_address newTblTempAddr = g_temporary_paging_util::map(newTblPhys);
					g_page_table newTbl = (g_page_table) newTblTempAddr;

					// copy page table entries
					for (uint32_t pi = 0; pi < 1024; pi++) {
						if (curTbl[pi]) {
							// map read-only
							newTbl[pi] = curTbl[pi] & ~G_PAGE_TABLE_READWRITE;
							g_pp_reference_tracker::increment(PAGE_ALIGN_DOWN(curTbl[pi]));
						} else {
							newTbl[pi] = 0;
						}
					}

					g_temporary_paging_util::unmap(newTblTempAddr);

					newDir[ti] = newTblPhys | curTblFlags;
				}
			} else {
				newDir[ti] = 0;
			}
		}

		newDir[0] = curDir[0]; // lowest 4 MiB
		newDir[1023] = newDirPhys | DEFAULT_KERNEL_TABLE_FLAGS; // recursive-ness

		// copy user stack
		g_physical_address newUserStackPhys = g_pp_allocator::allocate();
		g_address_space::map_to_temporary_mapped_directory(newDir, newUserStackVirt, newUserStackPhys, DEFAULT_USER_TABLE_FLAGS,
		DEFAULT_USER_PAGE_FLAGS, true);

		g_virtual_address newUserStackTemp = g_temporary_paging_util::map(newUserStackPhys);
		g_memory::copy((uint8_t*) newUserStackTemp, (uint8_t*) current->userStack, G_PAGE_SIZE);
		g_temporary_paging_util::unmap(newUserStackTemp);

		g_temporary_paging_util::unmap(newDirTempAddr);
	}
	// XXX

	// Copy kernel stack
	g_physical_address kernelStackPhys = g_pp_allocator::allocate();
	g_address_space::map(newKernelStackVirt, kernelStackPhys,
	DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
	g_memory::copy((uint8_t*) newKernelStackVirt, (uint8_t*) current->kernelStack, G_PAGE_SIZE);

	// Prepare current dir
	g_page_directory directory = (g_page_directory) G_CONST_RECURSIVE_PAGE_DIRECTORY_ADDRESS;
	for (uint32_t ti = 1; ti < 1023; ti++) {
		if (directory[ti]) {
			if (ti * 1024 * G_PAGE_SIZE < G_CONST_USER_VIRTUAL_RANGES_START) { // Process area
				// Make all pages of the current process read-only (CoW)
				g_page_table table = G_CONST_RECURSIVE_PAGE_TABLE(ti);
				for (uint32_t pi = 0; pi < 1024; pi++) {
					table[pi] = table[pi] & ~G_PAGE_TABLE_READWRITE;
				}

			} else if (ti * 1024 * G_PAGE_SIZE < G_CONST_KERNEL_AREA_START) { // Virtual ranges
				// TODO: what do with virtual ranges?

			}
		}
	}

	return newDirPhys;
}

/**
 *
 */
void g_thread_manager::prepareSpaceForThread(g_page_directory rootProcessPageDirectory, g_virtual_address kernelStack, g_virtual_address userStack) {
	// Physical addresses
	g_physical_address kernelStackPhys = g_pp_allocator::allocate();
	g_physical_address userStackPhys = g_pp_allocator::allocate();

	uint32_t pd = (uint32_t) rootProcessPageDirectory;

	// XXX TEMPORARY MAPPING XXX
	{
		g_virtual_address tmpPageDirVirt = g_temporary_paging_util::map(pd);

		g_page_directory tmpPageDir = (g_page_directory) tmpPageDirVirt;
		g_address_space::map_to_temporary_mapped_directory(tmpPageDir, userStack, userStackPhys, DEFAULT_USER_TABLE_FLAGS,
		DEFAULT_USER_PAGE_FLAGS);

		g_temporary_paging_util::unmap(tmpPageDirVirt);
	}
	// XXX

	// Map kernel stack
	g_address_space::map(kernelStack, kernelStackPhys,
	DEFAULT_KERNEL_TABLE_FLAGS, DEFAULT_KERNEL_PAGE_FLAGS);
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
g_thread* g_thread_manager::fork(g_thread* current) {

	g_process* parent = current->process;

	// Virtual target addresses
	g_virtual_address kernelStackVirt = g_kernel_virt_addr_ranges->allocate(1);
	g_virtual_address userStackVirt = G_CONST_KERNEL_AREA_START - G_PAGE_SIZE;
	g_physical_address pd = prepareSpaceForFork(current, kernelStackVirt, userStackVirt);

	/**
	 * Create the process
	 */
	g_virtual_address esp0 = kernelStackVirt + G_PAGE_SIZE;

	g_thread* thread = new g_thread(g_thread_type::THREAD_MAIN);
	thread->cpuState = (g_processor_state*) (esp0 - sizeof(g_processor_state));
	thread->kernelStackEsp0 = esp0;

	thread->kernelStack = kernelStackVirt;
	thread->userStack = userStackVirt;

	/**
	 * Create the process
	 */
	g_process* process = new g_process(parent->securityLevel);
	process->parent = parent;
	process->main = thread;
	process->pageDirectory = (uint32_t*) pd;
	thread->process = process;

	process->heapBreak = parent->heapBreak;
	process->heapPages = parent->heapPages;
	process->heapStart = parent->heapStart;
	process->imageEnd = parent->imageEnd;
	process->imageStart = parent->imageStart;

	// Forked process has no virtual ranges // TODO keep shared regions and stuff
	process->virtualRanges.initialize(G_CONST_USER_VIRTUAL_RANGES_START, userStackVirt);

#if G_LOGGING_DEBUG
	dumpTask(thread);
#endif
	return thread;
}

/**
 *
 */
g_thread* g_thread_manager::createProcess(g_security_level securityLevel) {

	// Virtual target addresses
	g_virtual_address kernelStackVirt = g_kernel_virt_addr_ranges->allocate(1);
	g_virtual_address userStackVirt = G_CONST_KERNEL_AREA_START - G_PAGE_SIZE;
	g_physical_address pd = prepareSpaceForProcess(kernelStackVirt, userStackVirt);

	/**
	 * Create the state
	 */
	g_virtual_address esp0 = kernelStackVirt + G_PAGE_SIZE;
	g_virtual_address esp = userStackVirt + G_PAGE_SIZE;

	g_processor_state* state = (g_processor_state*) (esp0 - sizeof(g_processor_state));
	g_memory::setBytes(state, 0, sizeof(g_processor_state));
	state->esp = esp;
	state->eip = 0;
	state->eflags = 0x200;

	// Apply process security level on thread
	applySecurityLevel(state, securityLevel);

	/**
	 * Create the main thread
	 */
	g_thread* thread = new g_thread(g_thread_type::THREAD_MAIN);
	thread->cpuState = state;
	thread->kernelStackEsp0 = esp0;

	thread->kernelStack = kernelStackVirt;
	thread->userStack = userStackVirt;

	/**
	 * Create the process
	 */
	g_process* process = new g_process(securityLevel);
	process->main = thread;
	process->pageDirectory = (uint32_t*) pd;
	thread->process = process;

	// Initialize the virtual range manager
	// (gets everything from "value of constant" to start of process stack)
	process->virtualRanges.initialize(G_CONST_USER_VIRTUAL_RANGES_START, userStackVirt);

#if G_LOGGING_DEBUG
	dumpTask(thread);
#endif
	return thread;
}

/**
 *
 */
g_thread* g_thread_manager::createProcessVm86(uint8_t interrupt, g_vm86_registers& in, g_vm86_registers* out) {

	// Virtual target addresses
	g_virtual_address kernelStackVirt = g_kernel_virt_addr_ranges->allocate(1);

	// Allocate a user stack in lower memory
	g_virtual_address userStackVirt = (uint32_t) g_lower_heap::allocate(0x2000);
	g_physical_address pageDirPhys = prepareSpaceForProcess(kernelStackVirt);

	/**
	 * Create the state
	 */
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

	/**
	 * Create the VM86 main thread
	 */
	g_thread* thread = new g_thread(g_thread_type::THREAD_VM86);
	thread->cpuState = (g_processor_state*) state;
	thread->kernelStackEsp0 = esp0;

	thread->kernelStack = kernelStackVirt;
	thread->userStack = userStackVirt;

	thread->getVm86Information()->out = out;

	/**
	 * Create the process
	 */
	g_process* process = new g_process(G_SECURITY_LEVEL_KERNEL);
	process->main = thread;
	process->pageDirectory = (uint32_t*) pageDirPhys;
	thread->process = process;

#if G_LOGGING_DEBUG
	dumpTask(thread);
#endif
	return thread;
}

/**
 *
 */
g_thread* g_thread_manager::createThread(g_process* process) {

	// Virtual target addresses
	g_virtual_address userStackVirt = process->virtualRanges.allocate(1);
	if (userStackVirt == 0) {
		g_log_warn("%! couldn't create thread in process %i, no free user ranges", "taskmgr", process->main->id);
		return 0;
	}

	g_virtual_address kernelStackVirt = g_kernel_virt_addr_ranges->allocate(1);
	prepareSpaceForThread(process->pageDirectory, kernelStackVirt, userStackVirt);

	/**
	 * Create the state
	 */
	g_virtual_address esp0 = kernelStackVirt + G_PAGE_SIZE;
	g_virtual_address esp = userStackVirt + G_PAGE_SIZE;

	g_processor_state* state = (g_processor_state*) (esp0 - sizeof(g_processor_state));
	g_memory::setBytes(state, 0, sizeof(g_processor_state));
	state->esp = esp;
	state->eip = 0;
	state->eflags = 0x200;
	// Apply security level
	applySecurityLevel(state, process->securityLevel);

	// Create the thread
	g_thread* thread = new g_thread(g_thread_type::THREAD);
	thread->cpuState = state;
	thread->kernelStackEsp0 = esp0;
	thread->kernelStack = kernelStackVirt;
	thread->userStack = userStackVirt;

	thread->process = process;

	// User-Thread (thread-local-storage etc.)
	prepareThreadLocalStorage(thread);

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

		// Calculate size that TLS needs including alignment
		uint32_t tls_master_aligned_total_size = G_ALIGN_UP(process->tls_master_totalsize, process->tls_master_alignment);

		// Allocate virtual range with aligned size of TLS + size of {g_user_thread}
		uint32_t required_size = tls_master_aligned_total_size + sizeof(g_user_thread);
		uint32_t required_pages = PAGE_ALIGN_UP(required_size) / G_PAGE_SIZE;
		g_virtual_address tls_copy_virt = process->virtualRanges.allocate(required_pages, G_PROC_VIRTUAL_RANGE_FLAG_PHYSICAL_OWNER);

		// Store executing space
		g_page_directory current = g_address_space::get_current_space();

		// Temporarily switch to target process directory, copy TLS contents
		g_address_space::switch_to_space(process->pageDirectory);
		for (uint32_t i = 0; i < required_pages; i++) {
			g_physical_address phys = g_pp_allocator::allocate();
			g_address_space::map(tls_copy_virt + i * G_PAGE_SIZE, phys,
			DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);
			g_pp_reference_tracker::increment(phys);
		}

		// Zero & copy TLS content
		g_memory::setBytes((void*) tls_copy_virt, 0, process->tls_master_totalsize);
		g_memory::copy((void*) tls_copy_virt, (void*) process->tls_master_in_proc_location, process->tls_master_copysize);

		// Fill user thread
		g_virtual_address user_thread_loc = tls_copy_virt + tls_master_aligned_total_size;
		g_user_thread* user_thread = (g_user_thread*) user_thread_loc;
		user_thread->self = user_thread;

		// Switch back
		g_address_space::switch_to_space(current);

		// Set threads TLS location
		thread->user_thread_addr = user_thread_loc;
		thread->tls_copy_virt = tls_copy_virt;

		g_log_debug("%! created tls copy in process %i, thread %i at %h", "taskmgr", process->main->id, thread->id, thread->tls_copy_virt);
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

	if (task->type == g_thread_type::THREAD) {

		// Here we free everything that the thread has created and that is no more
		// needed by anyone.
		g_process* process = task->process;

		// TODO

	} else if (task->type == g_thread_type::THREAD_MAIN) {

		// Here we free everything that the process has created and that is no more
		// needed by anyone.
		g_process* process = task->process;

		// tell the filesystem to clean up
		g_filesystem::process_closed(task->id);

		// TODO

	} else if (task->type == g_thread_type::THREAD_VM86) {

		// User stack is in memory < 1MB so don't unmap
		g_lower_heap::free((void*) task->userStack);

		// TODO:
		// figure out vm86 deletion
	}

	g_log_debug("%! task %i has died, now %i free phys pages", "taskmgr", task->id, g_pp_allocator::getFreePageCount());
	delete task;
	return;
}

/**
 *
 */
void g_thread_manager::dumpTask(g_thread* task) {
#if G_LOGGING_DEBUG
	g_log_debug("%! created %s %i", "taskmgr",
			(task->type == g_thread_type::THREAD ? "thread" : (task->type == g_thread_type::THREAD_VM86 ? "vm86 process" : "process")), task->id);

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
	if (task->type == g_thread_type::THREAD_VM86) {
		g_log_debug("%#  cs:ip:  %h:%h", task->cpuState->cs, task->cpuState->eip);
		g_log_debug("%#  ax: %h bx: %h cx: %h dx: %h", task->cpuState->eax, task->cpuState->ebx, task->cpuState->ecx, task->cpuState->edx);g_log_debug(
				"%#  user sp:  %h:%h", task->cpuState->esp, task->cpuState->ss);
	} else {
		g_log_debug("%#  user sp:       %h", task->cpuState->esp);g_log_debug("%#  entry point:   %h", task->cpuState->eip);
	}
#endif
}

