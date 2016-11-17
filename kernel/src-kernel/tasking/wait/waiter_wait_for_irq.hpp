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

#ifndef GHOST_MULTITASKING_WAIT_MANAGER_INTERRUPTS
#define GHOST_MULTITASKING_WAIT_MANAGER_INTERRUPTS

#include <system/interrupts/handling/interrupt_request_dispatcher.hpp>
#include <tasking/wait/waiter.hpp>

/**
 *
 */
class g_waiter_wait_for_irq: public g_waiter {
private:
	uint32_t interrupt;

public:
	g_waiter_wait_for_irq(uint32_t _interrupt) {
		this->interrupt = _interrupt;
	}

	/**
	 *
	 */
	virtual bool checkWaiting(g_thread* task) {
		bool fired = g_interrupt_request_dispatcher::poll_irq((uint32_t) interrupt);

		if (fired) {
			// Interrupt was fired
			return false;
		} else {
			// Keep waiting for the interrupt
			return true;
		}
	}

	/**
	 *
	 */
	virtual const char* debug_name() {
		return "wait-for-irq";
	}

};

#endif
