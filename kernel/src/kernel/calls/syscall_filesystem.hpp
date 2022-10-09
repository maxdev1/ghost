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

#ifndef __KERNEL_SYSCALL_FS__
#define __KERNEL_SYSCALL_FS__

#include "ghost/calls/calls.h"
#include "kernel/tasking/tasking.hpp"

void syscallFsOpen(g_task* task, g_syscall_fs_open* data);

void syscallFsSeek(g_task* task, g_syscall_fs_seek* data);

void syscallFsRead(g_task* task, g_syscall_fs_read* data);

void syscallFsWrite(g_task* task, g_syscall_fs_write* data);

void syscallFsClose(g_task* task, g_syscall_fs_close* data);

void syscallFsLength(g_task* task, g_syscall_fs_length* data);

void syscallFsTell(g_task* task, g_syscall_fs_tell* data);

void syscallFsStat(g_task* task, g_syscall_fs_stat* data);

void syscallFsFstat(g_task* task, g_syscall_fs_fstat* data);

void syscallFsCloneFd(g_task* task, g_syscall_fs_clonefd* data);

void syscallFsPipe(g_task* task, g_syscall_fs_pipe* data);

void syscallOpenIrqDevice(g_task* task, g_syscall_open_irq_device* data);

void syscallFsOpenDirectory(g_task* task, g_syscall_fs_open_directory* data);

void syscallFsReadDirectory(g_task* task, g_syscall_fs_read_directory* data);

void syscallFsCloseDirectory(g_task* task, g_syscall_fs_close_directory* data);

#endif
