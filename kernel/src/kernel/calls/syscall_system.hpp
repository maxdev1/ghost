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

#ifndef __KERNEL_SYSCALL_SYSTEM__
#define __KERNEL_SYSCALL_SYSTEM__

#include "kernel/tasking/tasking.hpp"
#include <ghost/system/callstructs.h>

void syscallLog(g_task* task, g_syscall_log* data);

void syscallSetVideoLog(g_task* task, g_syscall_set_video_log* data);

void syscallTest(g_task* task, g_syscall_test* data);

void syscallCallVm86(g_task* task, g_syscall_call_vm86* data);

void syscallIrqCreateRedirect(g_task* task, g_syscall_irq_create_redirect* data);

void syscallAwaitIrq(g_task* task, g_syscall_await_irq* data);

void syscallGetEfiFramebuffer(g_task* task, g_syscall_get_efi_framebuffer* data);

#endif
