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

#ifndef GHOST_API_CALLS_SPAWNINGCALLS
#define GHOST_API_CALLS_SPAWNINGCALLS

#include "ghost/kernel.h"
#include "ghost/system.h"
#include "ghost/ramdisk.h"

/**
 * @field path
 * 		absolute path of the binary
 *
 * @field securityLevel
 * 		target process security level
 *
 * @field spawnStatus
 * 		result of spawning
 */
typedef struct
{
	char* path;
	g_security_level securityLevel;
	const char* args;
	const char* workdir;
	g_fd inStdio[3];

	g_fd outStdio[3];
	g_pid pid;
	g_spawn_status spawnStatus;
	g_spawn_validation_details validationDetails;
}__attribute__((packed)) g_syscall_spawn;

/**
 * @field initialEntry
 * 		the initial thread entry
 *
 * @field userEntry
 * 		user-defined
 *
 * @field userData
 * 		user-defined
 *
 * @field status
 * 		result of thread creation
 */
typedef struct
{
	void* initialEntry;
	void* userEntry;
	void* userData;

	g_create_thread_status status;
	g_tid threadId;
}__attribute__((packed)) g_syscall_create_thread;

/**
 * @field userEntry
 * 		the user entry
 *
 * @field userData
 * 		the user data
 */
typedef struct
{
	void* userEntry;
	void* userData;
}__attribute__((packed)) g_syscall_get_thread_entry;

/**
 * Used for process configuration on spawning.
 */
typedef struct
{
	char* source_path;
}__attribute__((packed)) g_process_configuration;

/**
 * @field buffer
 * 		target buffer, with a size of at least
 * 		{PROCESS_COMMAND_LINE_ARGUMENTS_BUFFER_LENGTH}
 */
typedef struct
{
	char* buffer;
}__attribute__((packed)) g_syscall_cli_args_release;

/**
 * @field userThreadObject
 * 		pointer to the user thread object in TLS
 */
typedef struct
{
	void* userThreadObject;
}__attribute__((packed)) g_syscall_task_get_tls;

#endif
