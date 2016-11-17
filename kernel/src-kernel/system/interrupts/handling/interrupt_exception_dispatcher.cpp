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

#include "system/interrupts/handling/interrupt_exception_dispatcher.hpp"
#include <logger/logger.hpp>
#include <tasking/tasking.hpp>
#include <tasking/thread_manager.hpp>
#include <vm86/virtual_8086_monitor.hpp>
#include <system/system.hpp>
#include <memory/address_space.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/physical/pp_reference_tracker.hpp>
#include <memory/temporary_paging_util.hpp>
#include <memory/constants.hpp>

/**
 * Names of the exceptions
 */
static const char* EXCEPTION_NAMES[] = {
		"divide error", // 0x00
		"debug exception", // 0x01
		"non-maskable interrupt exception", // 0x02
		"breakpoint exception", // 0x03
		"detected overflow", // 0x04
		"out of bounds", // 0x05
		"invalid operation code", // 0x06
		"no co-processor", // 0x07
		"double fault", // 0x08
		"co-processor segment overrun", // 0x09
		"Bad TSS exception", // 0x0A
		"segment not present", // 0x0B
		"stack fault", // 0x0C
		"general protection fault", // 0x0D
		"page fault", // 0x0E
		"unknown interrupt", // 0x0F
		"co-processor fault", // 0x10
		"alignment check exception", // 0x11
		"machine check exception", // 0x12
		"reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception",
		"reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception", "reserved exception" // reserved exceptions
		};

/**
 *
 */
uint32_t g_interrupt_exception_dispatcher::getCR2() {
	uint32_t addr;
	asm volatile("mov %%cr2, %0" : "=r"(addr));
	return addr;
}

/**
 * Dumps the current CPU state to the log file
 */
void g_interrupt_exception_dispatcher::dump(g_thread* current_thread) {
	g_processor_state* cpuState = current_thread->cpuState;
	g_process* process = current_thread->process;
	g_log_info("%! %s in task %i (process %i)", "exception", EXCEPTION_NAMES[cpuState->intr], current_thread->id, process->main->id);

	if (current_thread->getIdentifier() != 0) {
		g_log_info("%# task identified as '%s'", current_thread->getIdentifier());
	}

	if (cpuState->intr == 0x0E) { // Page fault
		g_log_info("%#    accessed address: %h", getCR2());
	}
	g_log_info("%#    eip: %h   eflags: %h", cpuState->eip, cpuState->eflags);
	g_log_info("%#    eax: %h      ebx: %h", cpuState->eax, cpuState->ebx);
	g_log_info("%#    ecx: %h      edx: %h", cpuState->ecx, cpuState->edx);
	g_log_info("%#    esp: %h   state@: %h", cpuState->esp, cpuState);
	g_log_info("%#   intr: %h    error: %h", cpuState->intr, cpuState->error);
}

/**
 * Handles a general protection fault. If the current task is a VM86 task, the fault is redirected
 * to the Virtual8086 monitor.
 */
g_thread* g_interrupt_exception_dispatcher::handleGeneralProtectionFault(g_thread* current_thread) {

	if (current_thread->type == G_THREAD_TYPE_VM86) {

		g_virtual_monitor_handling_result result = g_virtual_8086_monitor::handleGpf(current_thread);

		if (result == VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL) {
			return current_thread;

		} else if (result == VIRTUAL_MONITOR_HANDLING_RESULT_FINISHED) {
			current_thread->alive = false;
			return g_tasking::schedule();

		} else if (result == VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE) {
			g_log_info("%! %i unable to handle gpf for vm86 task", "exception", g_system::currentProcessorId());
			current_thread->alive = false;
			return g_tasking::schedule();
		}
	}

	// Kill process, return with a switch
	g_thread* main = current_thread->process->main;
	main->alive = false;
	dump(current_thread);
	g_log_info("%! #%i process %i killed due to general protection fault", "exception", g_system::currentProcessorId(), main->id);
	return g_tasking::schedule();
}

/**
 *
 */
void g_interrupt_exception_dispatcher::printStackTrace(g_processor_state* state) {

	// This function is not very safe, use with caution.
	// Disallowed in anything but debug mode.
#if G_LOG_LEVEL == G_LOG_LEVEL_DEBUG
	g_log_info("%! Stack trace:", "exception");

	uint32_t * ebp = (uint32_t*) state->ebp;
	for (uint32_t frame = 0; frame < 5; ++frame) {
		uint32_t eip = ebp[1];
		if (eip == 0) {
			break;
		}

		ebp = reinterpret_cast<uint32_t*>(ebp[0]);
		// uint32_t* arguments = &ebp[2];
		g_log_info("%#  %h", eip);
	}
#endif
}

/**
 * Handles a page fault
 */
g_thread* g_interrupt_exception_dispatcher::handlePageFault(g_thread* current_thread) {

	g_virtual_address accessedVirtual = G_PAGE_ALIGN_DOWN(getCR2());
	g_physical_address accessedPhysical = g_address_space::virtual_to_physical(accessedVirtual);

	// handle-able stack overflow?
	if (current_thread->type == G_THREAD_TYPE_SUB || current_thread->type == G_THREAD_TYPE_MAIN) {

		// calculate the virtual stack area
		g_virtual_address stackAreaStart = current_thread->userStackAreaStart
				- (G_THREAD_USER_STACK_RESERVED_VIRTUAL_PAGES - current_thread->userStackPages) * G_PAGE_SIZE;
		g_virtual_address stackAreaEnd = current_thread->userStackAreaStart + current_thread->userStackPages * G_PAGE_SIZE;

		if (accessedVirtual >= stackAreaStart && accessedVirtual < stackAreaEnd) {

			// start at the accessed page
			g_virtual_address unmappedNext = G_PAGE_ALIGN_DOWN(accessedVirtual);
			while (unmappedNext < current_thread->userStackAreaStart) {
				// map physical pages until mapped stack-start is reached
				g_physical_address addPagePhys = g_pp_allocator::allocate();
				g_address_space::map(unmappedNext, addPagePhys, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS);

				unmappedNext += G_PAGE_SIZE;
				current_thread->userStackPages++;
			}

			// set area to new location
			current_thread->userStackAreaStart = G_PAGE_ALIGN_DOWN(accessedVirtual);

			// continue
			return current_thread;
		}
	}

	// Copy-on-write?
	// Check if within binary image range
	if (accessedVirtual >= current_thread->process->imageStart && accessedVirtual <= current_thread->process->imageEnd) {

		uint32_t ti = G_TABLE_IN_DIRECTORY_INDEX(accessedVirtual);
		uint32_t pi = G_PAGE_IN_TABLE_INDEX(accessedVirtual);
		g_page_table table = G_CONST_RECURSIVE_PAGE_TABLE(ti);
		if (table[pi] != 0) {

			// get new physical page
			g_physical_address newPhysPhysical = g_pp_allocator::allocate();
			// map it temporary
			g_virtual_address newPhysTemp = g_temporary_paging_util::map(newPhysPhysical);
			// copy contents
			g_memory::copy((uint8_t*) newPhysTemp, (uint8_t*) accessedVirtual, G_PAGE_SIZE);
			// write new mapping
			g_address_space::map(accessedVirtual, newPhysPhysical, DEFAULT_USER_TABLE_FLAGS, DEFAULT_USER_PAGE_FLAGS, true);
			// unmap temporary
			g_temporary_paging_util::unmap(newPhysTemp);

			// new physical page has one more reference, old one has one less
			g_pp_reference_tracker::increment(newPhysPhysical);

			if (g_pp_reference_tracker::decrement(accessedPhysical) == 0) {
				g_pp_allocator::free(accessedPhysical);
			}

			g_log_debug("%! (%i:%i) entry %i/%i copied", "cow", current_thread->process->main->id, current_thread->id, ti, pi);
			return current_thread;
		}
	}

	// raise SIGSEGV in thread
	current_thread->raise_signal(SIGSEGV);
	g_log_info("%! (core %i) raised SIGSEGV in thread %i", "pagefault", g_system::currentProcessorId(), current_thread->id);
	dump(current_thread);

	// old stuff:
	//  // Unhandled
	//  g_log_info("%# mapping: virtual %h, physical %h", accessedVirtual, accessedPhysical);
	//  // Kill process, return with a switch
	//  g_thread* main = thread->process->main;
	//  main->alive = false;

	return g_tasking::schedule();
}

/**
 * Handles a divide error
 */
g_thread* g_interrupt_exception_dispatcher::handleDivideError(g_thread* current_thread) {

	dump(current_thread);

	// Let process run, but skip the faulty instruction
	++current_thread->cpuState->eip;
	g_log_info("%! #%i thread %i had a divide error", "exception", g_system::currentProcessorId(), current_thread->id);
	return g_tasking::schedule();
}

/**
 * Handles a invalid operation code
 */
g_thread* g_interrupt_exception_dispatcher::handleInvalidOperationCode(g_thread* current_thread) {

	dump(current_thread);

	// kill thread and process, return with a switch
	g_thread* main = current_thread->process->main;
	main->alive = false;
	current_thread->alive = false;
	g_log_info("%! #%i process %i killed due to invalid operation code %h in thread %i", "exception", g_system::currentProcessorId(), main->id,
			*((uint8_t* ) current_thread->cpuState->eip), current_thread->id);
	return g_tasking::schedule();
}

/**
 *
 */
g_thread* g_interrupt_exception_dispatcher::handle(g_thread* current_thread) {

	g_logger::manualLock();

	bool resolved = false;

	switch (current_thread->cpuState->intr) {
	case 0x00: { // Divide error
		current_thread = handleDivideError(current_thread);
		resolved = true;
		break;
	}
	case 0x0E: { // Page fault
		current_thread = handlePageFault(current_thread);
		resolved = true;
		break;
	}
	case 0x0D: { // General protection fault
		current_thread = handleGeneralProtectionFault(current_thread);
		resolved = true;
		break;
	}
	case 0x06: { // Invalid operation code
		current_thread = handleInvalidOperationCode(current_thread);
		resolved = true;
		break;
	}
	}

	if (resolved) {
		g_logger::manualUnlock();
		return current_thread;
	}

	// No resolution
	g_log_info("%*%! no resolution, hanging system", 0x0C, "exception");
	dump(current_thread);
	for (;;) {
		asm("hlt");
	}
	return 0;
}
