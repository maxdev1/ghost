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

#ifndef GHOST_MULTITASKING_WAIT_MANAGER_VIRTUAL_8086
#define GHOST_MULTITASKING_WAIT_MANAGER_VIRTUAL_8086

#include "ghost/kernel.h"
#include <tasking/wait/waiter.hpp>

/**
 *
 */
class g_waiter_call_vm86: public g_waiter {
private:
	g_syscall_call_vm86* data;
	g_vm86_registers* temporaryOut;
	uint32_t virtual8086ProcessId;

public:
	g_waiter_call_vm86(g_syscall_call_vm86* _data, g_vm86_registers* _temporaryOut, uint32_t _virtual8086ProcessId) {
		this->data = _data;
		this->temporaryOut = _temporaryOut;
		this->virtual8086ProcessId = _virtual8086ProcessId;
	}

	/**
	 *
	 */
	virtual bool checkWaiting(g_thread* task) {

		g_thread* vm86task = g_tasking::getTaskById(virtual8086ProcessId);

		if (vm86task == 0) {
			// Task has finished, copy the values to the waiting data struct
			*(data->out) = *temporaryOut;

			// Delete temporary out struct
			delete temporaryOut;

			return false;
		} else {
			// Keep waiting for Vm86 to finish
			return true;
		}

	}

	/**
	 *
	 */
	virtual const char* debug_name() {
		return "vm86-call";
	}

};

#endif
