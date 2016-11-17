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

#include <logger/logger.hpp>
#include <system/io_ports.hpp>
#include <tasking/tasking.hpp>
#include <system/interrupts/pic.hpp>
#include <calls/syscall_handler.hpp>
#include <system/system.hpp>
#include <system/interrupts/ioapic_manager.hpp>
#include <memory/address_space.hpp>
#include <system/interrupts/handling/interrupt_request_dispatcher.hpp>
#include <system/interrupts/ioapic.hpp>
#include <system/interrupts/ioapic_manager.hpp>

#include <tasking/tasking.hpp>
#include <tasking/thread_manager.hpp>
#include <tasking/wait/waiter_join.hpp>

/**
 * Map denoting which IRQs have happened and should be handled.
 */
bool irqsWaiting[256] = { };
g_irq_handler* handlers[256] = { 0 };

/**
 *
 */
g_thread* g_interrupt_request_dispatcher::handle(g_thread* current_thread) {

	const uint32_t interrupt = current_thread->cpuState->intr;
	const uint32_t irq = interrupt - 0x20;

	if (current_thread->cpuState->intr == 0x80) {
		/* System call */
		current_thread = g_syscall_handle(current_thread);

	} else if (irq == 0xFF) {
		/* Spurious interrupt */
		g_log_warn("%! spurious interrupt was caught", "requests");

	} else if (irq == 0) {
		/* Timer interrupt */
		g_tasking::currentScheduler()->updateMilliseconds();
		current_thread = g_tasking::schedule();

	} else if (irq < 0xFF) {
		/* Other interrupt request */
		auto handler = handlers[irq];
		if (handler) {
			g_thread* thread = g_tasking::getTaskById(handler->thread_id);
			if (thread != nullptr) {
				// let the thread enter the irq handler
				thread->enter_irq_handler(handler->handler, irq, handler->callback);
				// it could be the current thread, so we have to switch
				current_thread = g_tasking::schedule();
			}

		} else {
			// mark the IRQ as occured
			irqsWaiting[irq] = true;
		}

	} else {
		g_log_warn("%! unknown request number %i", "irq", irq);
	}

	return current_thread;
}

/**
 * 
 */
bool g_interrupt_request_dispatcher::poll_irq(uint8_t irq) {

	if (irqsWaiting[irq]) {
		irqsWaiting[irq] = false;
		return true;
	}
	return false;
}

/**
 *
 */
void g_interrupt_request_dispatcher::set_handler(uint8_t irq, g_tid thread_id, uintptr_t handler_addr, uintptr_t callback_addr) {

	if (handlers[irq]) {
		delete handlers[irq];
	}

	g_irq_handler* handler = new g_irq_handler;
	handler->thread_id = thread_id;
	handler->handler = handler_addr;
	handler->callback = callback_addr;
	handlers[irq] = handler;
}

