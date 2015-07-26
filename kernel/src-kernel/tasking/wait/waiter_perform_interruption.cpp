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

#include <tasking/wait/waiter_perform_interruption.hpp>

/**
 *
 */
bool g_waiter_perform_interruption::checkWaiting(g_thread* task) {

	task->store_for_interruption();

	// set the new entry
	task->cpuState->eip = entry;

	// jump to next stack value
	uint32_t* esp = (uint32_t*) (task->cpuState->esp);

	// pass parameter value
	if (task->interruption_info->type == g_thread_interruption_info_type::IRQ) {
		// pass IRQ
		--esp;
		*esp = task->interruption_info->handled_irq;

	} else if (task->interruption_info->type == g_thread_interruption_info_type::SIGNAL) {
		// pass signal
		--esp;
		*esp = task->interruption_info->handled_signal;
	}

	// put callback as return address on stack
	--esp;
	*esp = callback;

	// set new ESP
	task->cpuState->esp = (uint32_t) esp;

	return false;
}

