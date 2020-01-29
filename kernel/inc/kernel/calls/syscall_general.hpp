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

#ifndef __KERNEL_SYSCALL_GENERAL__
#define __KERNEL_SYSCALL_GENERAL__

#include "ghost/calls/calls.h"
#include "kernel/tasking/tasking.hpp"

void syscallAtomicLock(g_task* task, g_syscall_atomic_lock* data);

void syscallLog(g_task* task, g_syscall_log* data);

void syscallSetVideoLog(g_task* task, g_syscall_set_video_log* data);

void syscallTest(g_task* task, g_syscall_test* data);

void syscallReleaseCliArguments(g_task* task, g_syscall_cli_args_release* data);

void syscallGetMilliseconds(g_task* task, g_syscall_millis* data);

void syscallGetExecutablePath(g_task* task, g_syscall_fs_get_executable_path* data);

void syscallGetWorkingDirectory(g_task* task, g_syscall_fs_get_working_directory* data);

void syscallSetWorkingDirectory(g_task* task, g_syscall_fs_set_working_directory* data);

void syscallKernQuery(g_task* task, g_syscall_fs_set_working_directory* data);

#endif
