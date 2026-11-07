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

#ifndef __KERNEL_SYSCALLS__
#define __KERNEL_SYSCALLS__

#include <ghost/stdint.h>

struct g_task;

/**
 * Type of a system call handler
 */
typedef void (*g_syscall_handler)(g_task*, void*);

#define G_SYSCALL_MAX_REQUEST_SIZE 256

/**
 * Defines the transfers performed for a fixed-size syscall request.
 */
enum g_syscall_access
{
    G_SYSCALL_INPUT,
    G_SYSCALL_OUTPUT,
    G_SYSCALL_IN_OUT
};

/**
 * Describes a syscall and its fixed request. A zero request size passes the
 * original address only to handlers that do not access a request.
 */
struct g_syscall_registration
{
    g_syscall_handler handler;
    bool interruptible;
    uint16_t requestSize;
    g_syscall_access access;
};

/**
 * Table of system call registrations with a size of G_SYSCALL_MAX.
 */
extern g_syscall_registration* syscallRegistrations;

/**
 * Handles a system call for the given task. Reads EAX and EBX from the caller task
 * to find the issued system call and data.
 *
 * Depending on the call registration, it is then processed either synchronously by
 * calling the respective handler or a thread is created and the source task is put
 * to waiting state.
 */
void syscallHandle(g_task* task);
void syscall(uint32_t callId, void* data);

/**
 * Classifies system calls that create a process or task.
 */
bool syscallStartsTask(uint32_t call);

/**
 * Classifies system calls that mutate task or filesystem state.
 */
bool syscallMutatesState(uint32_t call);

/**
 * Creates the system call table.
 */
void syscallRegisterAll();

#endif
