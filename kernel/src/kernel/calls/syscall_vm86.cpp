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

#include "kernel/calls/syscall_vm86.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/wait.hpp"
#include "shared/logger/logger.hpp"

void syscallCallVm86(g_task* task, g_syscall_call_vm86* data)
{
	if (task->securityLevel > G_SECURITY_LEVEL_DRIVER)
	{
		data->status = G_VM86_CALL_STATUS_FAILED_NOT_PERMITTED;
		return;
	}

	g_vm86_registers* registerStore = (g_vm86_registers*) heapAllocate(sizeof(g_vm86_registers));

	g_task* vm86task = taskingCreateThreadVm86(task->process, data->interrupt, data->in, registerStore);
	taskingAssign(taskingGetLocal(), vm86task);
	waitForVm86(task, vm86task, registerStore);
	taskingSchedule();
}

