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

#ifndef __GHOST_SYS_SYSTEM__
#define __GHOST_SYS_SYSTEM__

#include "ghost/common.h"
#include "ghost/kernel.h"
#include "ghost/fs.h"

__BEGIN_C

/**
 * Required to provide the <g_spawn> function. The spawning process shall
 * register itself with this identifier to be accessible via ipc messaging.
 */
#define G_SPAWNER_IDENTIFIER		"spawner"

// spawner commands
#define G_SPAWN_COMMAND_SPAWN_REQUEST	1
#define G_SPAWN_COMMAND_SPAWN_RESPONSE	2

// status codes for spawning
typedef uint8_t g_spawn_status;
#define G_SPAWN_STATUS_SUCCESSFUL 						((g_spawn_status) 0)
#define G_SPAWN_STATUS_IO_ERROR							((g_spawn_status) 1)
#define G_SPAWN_STATUS_MEMORY_ERROR						((g_spawn_status) 2)
#define G_SPAWN_STATUS_FORMAT_ERROR						((g_spawn_status) 3)
#define G_SPAWN_STATUS_TASKING_ERROR					((g_spawn_status) 4)
#define G_SPAWN_STATUS_DEPENDENCY_ERROR					((g_spawn_status) 5)
#define G_SPAWN_STATUS_DEPENDENCY_DUPLICATE				((g_spawn_status) 6)

typedef uint8_t g_spawn_validation_details;
#define G_SPAWN_VALIDATION_SUCCESSFUL				((g_spawn_validation_details) 0)
#define G_SPAWN_VALIDATION_ELF32_NOT_ELF			((g_spawn_validation_details) 1)
#define G_SPAWN_VALIDATION_ELF32_NOT_EXECUTABLE		((g_spawn_validation_details) 2)
#define G_SPAWN_VALIDATION_ELF32_NOT_I386			((g_spawn_validation_details) 3)
#define G_SPAWN_VALIDATION_ELF32_NOT_32BIT			((g_spawn_validation_details) 4)
#define G_SPAWN_VALIDATION_ELF32_NOT_LITTLE_ENDIAN	((g_spawn_validation_details) 5)
#define G_SPAWN_VALIDATION_ELF32_NOT_STANDARD_ELF	((g_spawn_validation_details) 6)
#define G_SPAWN_VALIDATION_ELF32_IO_ERROR			((g_spawn_validation_details) 7)

// command structs
typedef struct {
	int command;
}__attribute__((packed)) g_spawn_command_header;

typedef struct {
	g_spawn_command_header header;
	g_security_level security_level;
	size_t path_bytes;
	size_t args_bytes;
	size_t workdir_bytes;
	g_fd stdin;
	g_fd stdout;
	g_fd stderr;
	// followed by: path, args, workdir
}__attribute__((packed)) g_spawn_command_spawn_request;

typedef struct {
	g_spawn_status status;
	g_pid spawned_process_id;
	g_fd stdin_write;
	g_fd stdout_read;
	g_fd stderr_read;
}__attribute__((packed)) g_spawn_command_spawn_response;

// process configuration buffer lengths
#define G_CLIARGS_BUFFER_LENGTH			1024
#define G_CLIARGS_SEPARATOR				0x1F // ASCII character: UNIT SEPARATOR

// for <g_register_irq_handler>
typedef uint8_t g_register_irq_handler_status;
#define G_REGISTER_IRQ_HANDLER_STATUS_SUCCESSFUL		((g_register_irq_handler_status) 0)
#define G_REGISTER_IRQ_HANDLER_STATUS_NOT_PERMITTED		((g_register_irq_handler_status) 1)
#define G_REGISTER_IRQ_HANDLER_STATUS_INVALID_IRQ		((g_register_irq_handler_status) 2)

// for <g_register_signal_handler>
typedef uint8_t g_register_signal_handler_status;
#define G_REGISTER_SIGNAL_HANDLER_STATUS_SUCCESSFUL		((g_register_signal_handler_status) 0)
#define G_REGISTER_SIGNAL_HANDLER_STATUS_INVALID_SIGNAL ((g_register_signal_handler_status) 1)

// for <g_raise_signal>
typedef uint8_t g_raise_signal_status;
#define G_RAISE_SIGNAL_STATUS_SUCCESSFUL				((g_raise_signal_status) 0)
#define G_RAISE_SIGNAL_STATUS_INVALID_SIGNAL 			((g_raise_signal_status) 1)
#define G_RAISE_SIGNAL_STATUS_INVALID_TARGET 			((g_raise_signal_status) 2)
#define G_RAISE_SIGNAL_STATUS_INVALID_STATE 			((g_raise_signal_status) 3)

// for <g_kill>
typedef uint8_t g_kill_status;
#define G_KILL_STATUS_SUCCESSFUL						((g_kill_status) 0)
#define G_KILL_STATUS_NOT_FOUND							((g_kill_status) 1)
#define G_KILL_STATUS_FAILED					 		((g_kill_status) 2)

// for <g_create_thread>
typedef uint8_t g_create_thread_status;
#define G_CREATE_THREAD_STATUS_SUCCESSFUL				((g_create_thread_status) 0)
#define G_CREATE_THREAD_STATUS_FAILED					((g_create_thread_status) 1)

__END_C

#endif
