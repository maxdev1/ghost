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

#include "ghost/calls/calls.h"
#include "ghost/kernel.h"
#include <calls/syscall_handler.hpp>
#include <logger/logger.hpp>

#include <kernel.hpp>
#include <ramdisk/ramdisk.hpp>
#include <tasking/thread.hpp>
#include <tasking/tasking.hpp>
#include <tasking/thread_manager.hpp>
#include <tasking/wait/waiter_call_vm86.hpp>
#include <memory/address_space.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/physical/pp_reference_tracker.hpp>
#include <executable/elf32_loader.hpp>

G_SYSCALL_HANDLER(call_vm86) {

	g_syscall_call_vm86* data = (g_syscall_call_vm86*) G_SYSCALL_DATA(current_thread->cpuState);

	if (current_thread->process->securityLevel <= G_SECURITY_LEVEL_DRIVER) {
		// Copy in registers
		g_vm86_registers in = data->in;

		// Create temporary out struct
		g_vm86_registers* temporaryOut = new g_vm86_registers;

		// Create task
		g_thread* vm86task = g_thread_manager::createProcessVm86(data->interrupt, in, temporaryOut);
		g_tasking::addTask(vm86task);
		g_log_debug("%! task %i creates vm86 task %i to call interrupt %h", "syscall", current_thread->id, vm86task->id, data->interrupt);

		// Set wait
		current_thread->wait(new g_waiter_call_vm86(data, temporaryOut, vm86task->id));

		data->status = G_VM86_CALL_STATUS_SUCCESSFUL;
	} else {
		g_log_warn("%! task %i tried to do vm86 call but is not permitted", "syscall", current_thread->id);
		data->status = G_VM86_CALL_STATUS_FAILED_NOT_PERMITTED;
	}
	return g_tasking::schedule();
}

