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

#ifndef GHOST_API_CALLS_TASKINGCALLS
#define GHOST_API_CALLS_TASKINGCALLS

#include "ghost/kernel.h"
#include "ghost/system.h"

/**
 * @field code
 * 		the exit code
 */
typedef struct {
	uint32_t code;
}__attribute__((packed)) g_syscall_exit;

/**
 * @field pid
 * 		id of the process to kill
 */
typedef struct {
	g_pid pid;

	g_kill_status status;
}__attribute__((packed)) g_syscall_kill;

/**
 * @field id
 * 		the resulting id
 */
typedef struct {
	g_pid id;
}__attribute__((packed)) g_syscall_get_pid;

/**
 * @field id
 * 		the resulting id
 */
typedef struct {
	g_pid pid;

	g_pid parent_pid;
}__attribute__((packed)) g_syscall_get_parent_pid;

/**
 * @field id
 * 		the resulting id
 */
typedef struct {
	g_tid id;
}__attribute__((packed)) g_syscall_get_tid;

/**
 * @field id
 * 		the resulting id
 */
typedef struct {
	g_tid tid;

	g_pid pid;
}__attribute__((packed)) g_syscall_get_pid_for_tid;

/**
 * @field milliseconds
 * 		the number of milliseconds to sleep
 */
typedef struct {
	uint64_t milliseconds;
}__attribute__((packed)) g_syscall_sleep;

/**
 * @field irq
 * 		the IRQ to wait for
 */
typedef struct {
	uint8_t irq;
}__attribute__((packed)) g_syscall_wait_for_irq;

/**
 * @field atom_#
 * 		the atoms
 *
 * @field set_on_finish
 * 		whether or not to set the atom once finished
 *
 * @field try_only
 * 		whether or not to only try locking the atom
 *
 * @field was_set
 * 		whether the atom was set, when trying only
 */
typedef struct {
	g_atom* atom_1;
	g_atom* atom_2;
	uint8_t set_on_finish :1;
	uint8_t is_try :1;
	uint8_t has_timeout :1;
	uint64_t timeout;

	uint8_t was_set :1;
	uint8_t timed_out :1;
}__attribute__((packed)) g_syscall_atomic_lock;

/**
 * @field identifier
 * 		the identifier
 *
 * @field successful
 * 		whether the registration was successful
 */
typedef struct {
	char* identifier;

	uint8_t successful;
}__attribute__((packed)) g_syscall_task_id_register;

/**
 * @field identifier
 * 		the identifier
 *
 * @field resultTaskId
 * 		the task id, or G_TID_NONE if not successful
 */
typedef struct {
	char* identifier;

	g_tid resultTaskId;
}__attribute__((packed)) g_syscall_task_id_get;

/**
 * @field code
 * 		the exit code
 */
typedef struct {
	uint64_t millis;
}__attribute__((packed)) g_syscall_millis;

/**
 * @field forkedId
 * 		id of the forked process
 */
typedef struct {
	g_pid forkedId;
}__attribute__((packed)) g_syscall_fork;

/**
 * @field taskId
 * 		id of the task to wait for
 */
typedef struct {
	g_tid taskId;
}__attribute__((packed)) g_syscall_join;

/**
 * @field irq
 * 		irq to register for
 * @field handler
 * 		address of the handler function
 * @field callback
 * 		address of the function to be returned to when
 * 		the handler function returns
 * @field status
 * 		result of the command
 */
typedef struct {
	uint8_t irq;
	g_virtual_address handlerAddress;
	g_virtual_address returnAddress;
	g_register_irq_handler_status status;
}__attribute__((packed)) g_syscall_register_irq_handler;

/**
 * @field signal
 * 		signal to register for
 * @field handlerAddress
 * 		address of the handler function
 * @field callbackAddress
 * 		address of the function to be returned to when
 * 		the handler function returns
 * @field previousHandlerAddress
 * 		address of the previously registered handler
 * @field status
 * 		result of the command
 */
typedef struct {
	int signal;
	g_virtual_address handlerAddress;
	g_virtual_address returnAddress;
	g_virtual_address previousHandlerAddress;
	g_register_signal_handler_status status;
}__attribute__((packed)) g_syscall_register_signal_handler;

/**
 * @field signal
 * 		signal to raise
 * @field process
 * 		target process
 * @field status
 * 		result of the command
 */
typedef struct {
	int signal;
	g_pid process;
	g_raise_signal_status status;
}__attribute__((packed)) g_syscall_raise_signal;

/**
 * @field processInfo
 * 		pointer to the process info
 */
typedef struct {
	g_process_info* processInfo;
}__attribute__((packed)) g_syscall_process_get_info;

#endif
