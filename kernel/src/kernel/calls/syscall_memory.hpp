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

#ifndef __KERNEL_SYSCALL_MEMORY__
#define __KERNEL_SYSCALL_MEMORY__

#include "ghost/calls/calls.h"
#include "kernel/tasking/tasking.hpp"

void syscallSbrk(g_task* task, g_syscall_sbrk* data);

void syscallLowerMemoryAllocate(g_task* task, g_syscall_lower_malloc* data);

void syscallLowerMemoryFree(g_task* task, g_syscall_lower_free* data);

void syscallAllocateMemory(g_task* task, g_syscall_alloc_mem* data);

void syscallUnmap(g_task* task, g_syscall_unmap* data);

void syscallShareMemory(g_task* task, g_syscall_share_mem* data);

void syscallMapMmioArea(g_task* task, g_syscall_map_mmio* data);

#endif
