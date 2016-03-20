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

#include "tasking/thread.hpp"
#include "tasking/process.hpp"
#include "utils/string.hpp"
#include "kernel.hpp"
#include "logger/logger.hpp"
#include "tasking/wait/waiter_perform_interruption.hpp"
#include "debug/debug_interface_kernel.hpp"

static g_tid taskIdCounter = 0;

/**
 *
 */
g_thread::g_thread(g_thread_type _type) {

	id = taskIdCounter++;
	type = _type;
	priority = G_THREAD_PRIORITY_NORMAL;

	waitCount = 0;
	rounds = 0;
	scheduler = 0;

	alive = true;
	cpuState = 0;
	identifier = 0;

	process = 0;

	user_thread_addr = 0;
	userData = 0;
	threadEntry = 0;

	tls_copy_virt = 0;

	userStackAreaStart = 0;
	kernelStackPageVirt = 0;
	kernelStackEsp0 = 0;

	waitManager = 0;

	interruption_info = 0;

	if (type == G_THREAD_TYPE_VM86) {
		vm86Information = new g_thread_information_vm86();
	} else {
		vm86Information = 0;
	}
}

/**
 *
 */
g_thread::~g_thread() {

	if (identifier) {
		delete identifier;
	}

	if (waitManager) {
		delete waitManager;
	}

	if (vm86Information) {
		delete vm86Information;
	}

	if (interruption_info) {
		delete interruption_info;
	}
}

/**
 *
 */
void g_thread::setIdentifier(const char* newIdentifier) {

	if (identifier) {
		delete identifier;
	}

	uint32_t identLen = g_string::length(newIdentifier);
	identifier = new char[identLen + 1];
	g_memory::copy(identifier, newIdentifier, identLen);
	identifier[identLen] = 0;

	G_DEBUG_INTERFACE_TASK_SET_IDENTIFIER(this->id, identifier);
}

/**
 *
 */
const char* g_thread::getIdentifier() {
	return identifier;
}

/**
 *
 */
g_thread_information_vm86* g_thread::getVm86Information() {
	if (type == G_THREAD_TYPE_VM86 && vm86Information) {
		return vm86Information;
	}

	g_kernel::panic("tried to retrieve vm86 information from the non-vm86-process type task %i", id);
	return 0; // not reached
}

/**
 *
 */
bool g_thread::checkWaiting() {
	return waitManager->checkWaiting(this);
}

/**
 *
 */
void g_thread::wait(g_waiter* newWaitManager) {

	if (waitManager) {
		// replace waiter
		delete waitManager;
	} else {
		scheduler->moveToWaitQueue(this);
	}

	waitManager = newWaitManager;
	G_DEBUG_INTERFACE_TASK_SET_STATUS(this->id, newWaitManager->debug_name());
}

/**
 *
 */
void g_thread::unwait() {

	G_DEBUG_INTERFACE_TASK_SET_STATUS(this->id, "normal");
	if (waitManager) {
		delete waitManager;
		waitManager = 0;
		scheduler->moveToRunQueue(this);
	}
}

/**
 *
 */
bool g_thread::start_prepare_interruption() {

	// don't try to interrupt twice
	if (interruption_info != nullptr) {
		return false;
	}

	// create interruption info struct and store the waiter
	interruption_info = new g_thread_interruption_info();
	interruption_info->waitManager = waitManager;

	return true;
}

/**
 *
 */
void g_thread::finish_prepare_interruption(uintptr_t address, uintptr_t callback) {

	// append the waiter that does interruption
	waitManager = 0;
	wait(new g_waiter_perform_interruption(address, callback));

	// the next time this thread is regularly scheduled, the waiter
	// will store the state and do interruption
}

/**
 *
 */
void g_thread::enter_irq_handler(uintptr_t address, uint8_t irq, uintptr_t callback) {

	if (!start_prepare_interruption()) {
		return;
	}

	// tell interruption info that it's about an irq
	interruption_info->type = g_thread_interruption_info_type::IRQ;
	interruption_info->handled_irq = irq;

	G_DEBUG_INTERFACE_TASK_SET_STATUS(this->id, "irq-handling");
	finish_prepare_interruption(address, callback);
}

/**
 *
 */
void g_thread::enter_signal_handler(uintptr_t address, int signal, uintptr_t callback) {

	if (!start_prepare_interruption()) {
		return;
	}

	// tell interruption info that it's about an irq
	interruption_info->type = g_thread_interruption_info_type::SIGNAL;
	interruption_info->handled_signal = signal;

	G_DEBUG_INTERFACE_TASK_SET_STATUS(this->id, "signal-handling");
	finish_prepare_interruption(address, callback);
}

/**
 *
 */
void g_thread::store_for_interruption() {

	// store CPU state & it's stack location
	interruption_info->cpuState = *cpuState;
	interruption_info->cpuStateAddress = cpuState;

}

/**
 *
 */
void g_thread::restore_interrupted_state() {

	// set the waiter that was on the thread before interruption
	wait(interruption_info->waitManager);

	// restore CPU state
	cpuState = interruption_info->cpuStateAddress;
	*cpuState = interruption_info->cpuState;

	// remove interruption info
	delete interruption_info;
	interruption_info = 0;
	G_DEBUG_INTERFACE_TASK_SET_STATUS(this->id, "quitting-interruption");
}

/**
 *
 */
void g_thread::raise_signal(int signal) {

	// get handler from process
	g_signal_handler* handler = &(process->signal_handlers[signal]);
	if (handler->handler) {

		// get the thread that handles the signal (take this if its the right one)
		g_thread* handling_thread = 0;
		if (handler->thread_id == this->id) {
			handling_thread = this;
		} else {
			handling_thread = g_tasking::getTaskById(handler->thread_id);
		}

		// let handling thread enter signal handler
		if (handling_thread) {
			handling_thread->enter_signal_handler(handler->handler, signal, handler->callback);
		}
	} else {

		// do default handling if no handler is registered
		bool kill = false;

		if (signal == SIGSEGV) {
			kill = true;
			G_DEBUG_INTERFACE_TASK_SET_STATUS(this->id, "sigsegv");
		}

		if (kill) {
			g_log_info("%! thread %i killed", "signal", id);
			alive = false;
			process->main->alive = false;
		}
	}

}
