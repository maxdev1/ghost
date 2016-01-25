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

#include <system/interrupts/handling/interrupt_exception_handler.hpp>
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
uint32_t g_interrupt_exception_handler::getCR2() {
	uint32_t addr;
	asm volatile("mov %%cr2, %0" : "=r"(addr));
	return addr;
}

/**
 * Dumps the current CPU state to the log file
 */
void g_interrupt_exception_handler::dump(g_processor_state* cpuState) {
	g_thread* task = g_tasking::getCurrentThread();
	g_process* process = task->process;
	g_log_info("%! %s in task %i (process %i)", "exception", EXCEPTION_NAMES[cpuState->intr], task->id, process->main->id);

	if (task->getIdentifier() != 0) {
		g_log_info("%# task identified as '%s'", task->getIdentifier());
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
g_processor_state* g_interrupt_exception_handler::handleGeneralProtectionFault(g_processor_state* cpuState) {

	g_thread* task = g_tasking::getCurrentThread();

	if (task->type == g_thread_type::THREAD_VM86) {

		g_virtual_monitor_handling_result result = g_virtual_8086_monitor::handleGpf((g_processor_state_vm86*) cpuState);

		if (result == VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL) {
			return cpuState;

		} else if (result == VIRTUAL_MONITOR_HANDLING_RESULT_FINISHED) {
			task->alive = false;
			return g_tasking::schedule(cpuState);

		} else if (result == VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE) {
			g_log_info("%! %i unable to handle gpf for vm86 task", "exception", g_system::currentProcessorId());
			task->alive = false;
			return g_tasking::schedule(cpuState);
		}
	}

	// Kill process, return with a switch
	g_thread* main = task->process->main;
	main->alive = false;
	dump(cpuState);
	g_log_info("%! #%i process %i killed due to general protection fault", "exception", g_system::currentProcessorId(), main->id);
	return g_tasking::schedule(cpuState);
}

/**
 *
 */
void g_interrupt_exception_handler::printStackTrace(g_processor_state* state) {

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
g_processor_state* g_interrupt_exception_handler::handlePageFault(g_processor_state* cpuState) {

	g_thread* thread = g_tasking::getCurrentThread();
	g_virtual_address accessedVirtual = PAGE_ALIGN_DOWN(getCR2());
	g_physical_address accessedPhysical = g_address_space::virtual_to_physical(accessedVirtual);

	// Copy-on-write?
	// Check if within binary image range
	if (accessedVirtual >= thread->process->imageStart && accessedVirtual <= thread->process->imageEnd) {

		uint32_t ti = TABLE_IN_DIRECTORY_INDEX(accessedVirtual);
		uint32_t pi = PAGE_IN_TABLE_INDEX(accessedVirtual);
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

			g_log_debug("%! (%i:%i) entry %i/%i copied", "cow", thread->process->main->id, thread->id, ti, pi);
			return cpuState;
		}
	}

	// raise SIGSEGV in thread
	thread->raise_signal(SIGSEGV);
	g_log_info("%! (core %i) raised SIGSEGV in thread %i", "pagefault", g_system::currentProcessorId(), thread->id);
	dump(cpuState);

	// old stuff:
	//  // Unhandled
	//  g_log_info("%# mapping: virtual %h, physical %h", accessedVirtual, accessedPhysical);
	//  // Kill process, return with a switch
	//  g_thread* main = thread->process->main;
	//  main->alive = false;

	return g_tasking::schedule(cpuState);
}

/**
 * Handles a divide error
 */
g_processor_state* g_interrupt_exception_handler::handleDivideError(g_processor_state* cpuState) {

	dump(cpuState);

	// Let process run, but skip the faulty instruction
	g_thread* current = g_tasking::getCurrentThread();
	++current->cpuState->eip;
	g_log_info("%! #%i process %i had a divide error", "exception", g_system::currentProcessorId(), current->id);
	return g_tasking::schedule(cpuState);
}

/**
 * Handles a invalid operation code
 */
g_processor_state* g_interrupt_exception_handler::handleInvalidOperationCode(g_processor_state* cpuState) {

	dump(cpuState);

	// Kill process, return with a switch
	g_thread* task = g_tasking::getCurrentThread();
	g_thread* main = task->process->main;
	main->alive = false;
	g_log_info("%! #%i process %i killed due to invalid operation code %h", "exception", g_system::currentProcessorId(), main->id, *((uint8_t* ) cpuState->eip));
	return g_tasking::schedule(cpuState);
}

/**
 *
 */
g_processor_state* g_interrupt_exception_handler::handle(g_processor_state* cpuState) {

	g_logger::manualLock();

	bool resolved = false;

	switch (cpuState->intr) {
	case 0x00: { // Divide error
		cpuState = handleDivideError(cpuState);
		resolved = true;
		break;
	}
	case 0x0E: { // Page fault
		cpuState = handlePageFault(cpuState);
		resolved = true;
		break;
	}
	case 0x0D: { // General protection fault
		cpuState = handleGeneralProtectionFault(cpuState);
		resolved = true;
		break;
	}
	case 0x06: { // Invalid operation code
		cpuState = handleInvalidOperationCode(cpuState);
		resolved = true;
		break;
	}
	}

	if (resolved) {
		g_logger::manualUnlock();
		return cpuState;
	}

	// No resolution
	g_log_info("%*%! no resolution, hanging system", 0x0C, "exception");
	dump(cpuState);
	for (;;) {
		asm("hlt");
	}
	return 0;
}
