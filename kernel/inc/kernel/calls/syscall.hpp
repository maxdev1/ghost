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

struct g_task;

/**
 * Type of a system call handler
 */
typedef void (*g_syscall_handler)(g_task*, void*);

/**
 * Registration structure
 */
struct g_syscall_registration
{
	g_syscall_handler handler;
	bool threaded;
};

/**
 * Handles a system call for the given task. Reads EAX and EBX from the caller task
 * to find the issued system call and data.
 *
 * Depending on the call registration, it is then processed either synchronously by
 * calling the respective handler or a thread is created and the source task is put
 * to waiting state.
 */
void syscallHandle(g_task* task);

/**
 * Executes a system call within a thread. This creates or reuses the system call task on the
 * caller task and starts it within syscallThreadEntry.
 */
void syscallRunThreaded(g_syscall_handler handler, g_task* caller, void* syscallData);

/**
 * Entry point for threaded system call processing. Reads the system call information from the
 * source task and enters the system call handler accordingly.
 */
void syscallThreadEntry();

/**
 * Creates a system call registration.
 */
void syscallRegister(int call, g_syscall_handler handler, bool threaded);

/**
 * Creates the system call table.
 */
void syscallRegisterAll();

#endif

