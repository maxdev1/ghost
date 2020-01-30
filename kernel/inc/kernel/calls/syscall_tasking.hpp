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

#ifndef __KERNEL_SYSCALL_TASKING__
#define __KERNEL_SYSCALL_TASKING__

#include "ghost/calls/calls.h"
#include "kernel/tasking/tasking.hpp"

void syscallExit(g_task* task, g_syscall_exit* data);

void syscallYield(g_task* task);

void syscallGetProcessId(g_task* task, g_syscall_get_pid* data);

void syscallGetTaskId(g_task* task, g_syscall_get_tid* data);

void syscallGetProcessIdForTaskId(g_task* task, g_syscall_get_pid_for_tid* data);

void syscallFork(g_task* task, g_syscall_fork* data);

void syscallJoin(g_task* task, g_syscall_join* data);

void syscallSleep(g_task* task, g_syscall_sleep* data);

void syscallRegisterSignalHandler(g_task* task, g_syscall_register_signal_handler* data);

void syscallRegisterIrqHandler(g_task* task, g_syscall_register_irq_handler* data);

void syscallRestoreInterruptedState(g_task* task);

void syscallRaiseSignal(g_task* task, g_syscall_raise_signal* data);

void syscallSpawn(g_task* task, g_syscall_spawn* data);

void syscallTaskGetTls(g_task* task, g_syscall_task_get_tls* data);

void syscallProcessGetInfo(g_task* task, g_syscall_process_get_info* data);

void syscallKill(g_task* task, g_syscall_kill* data);

void syscallGetParentProcessId(g_task* task, g_syscall_get_parent_pid* data);

void syscallCreateThread(g_task* task, g_syscall_create_thread* data);

void syscallGetThreadEntry(g_task* task, g_syscall_get_thread_entry* data);

#endif
