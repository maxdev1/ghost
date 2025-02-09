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

#ifndef GHOST_API_TASKS_CALLSTRUCTS
#define GHOST_API_TASKS_CALLSTRUCTS

#include "../common.h"
#include "../stdint.h"
#include "types.h"

__BEGIN_C

/**
 * @field code
 * 		the exit code
 */
typedef struct
{
	uint32_t code;
} __attribute__((packed)) g_syscall_exit;

/**
 * @field pid
 * 		id of the process to kill
 */
typedef struct
{
	g_pid pid;

	g_kill_status status;
} __attribute__((packed)) g_syscall_kill;

/**
 * @field id
 * 		the resulting id
 */
typedef struct
{
	g_pid id;
} __attribute__((packed)) g_syscall_get_pid;

/**
 * @field id
 * 		the resulting id
 */
typedef struct
{
	g_pid pid;

	g_pid parent_pid;
} __attribute__((packed)) g_syscall_get_parent_pid;

/**
 * @field id
 * 		the resulting id
 */
typedef struct
{
	g_tid id;
} __attribute__((packed)) g_syscall_get_tid;

/**
 * @field id
 * 		the resulting id
 */
typedef struct
{
	g_tid tid;

	g_pid pid;
} __attribute__((packed)) g_syscall_get_pid_for_tid;

/**
 * @field milliseconds
 * 		the number of milliseconds to sleep
 */
typedef struct
{
	uint64_t milliseconds;
} __attribute__((packed)) g_syscall_sleep;

/**
 * @field irq
 * 		the IRQ to wait for
 */
typedef struct
{
	uint8_t irq;
} __attribute__((packed)) g_syscall_wait_for_irq;

/**
 * @field identifier
 * 		the identifier
 *
 * @field successful
 * 		whether the registration was successful
 */
typedef struct
{
	char* identifier;

	uint8_t successful;
} __attribute__((packed)) g_syscall_task_id_register;

/**
 * @field identifier
 * 		the identifier
 *
 * @field resultTaskId
 * 		the task id, or G_TID_NONE if not successful
 */
typedef struct
{
	char* identifier;

	g_tid resultTaskId;
} __attribute__((packed)) g_syscall_task_id_get;

/**
 * @field code
 * 		the exit code
 */
typedef struct
{
	uint64_t millis;
} __attribute__((packed)) g_syscall_millis;

/**
 * @field forkedId
 * 		id of the forked process
 */
typedef struct
{
	g_pid forkedId;
} __attribute__((packed)) g_syscall_fork;

/**
 * @field taskId
 * 		id of the task to wait for
 */
typedef struct
{
	g_tid taskId;
} __attribute__((packed)) g_syscall_join;


/**
 * @field processInfo
 * 		pointer to the process info
 */
typedef struct
{
	g_process_info* processInfo;
} __attribute__((packed)) g_syscall_process_get_info;

/**
 * @field path
 * 		absolute path of the binary
 *
 * @field securityLevel
 * 		target process security level
 *
 * @field status
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
	g_spawn_status status;
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

	g_create_task_status status;
	g_tid threadId;
}__attribute__((packed)) g_syscall_create_task;

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
}__attribute__((packed)) g_syscall_get_task_entry;

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
 * @field userThreadLocal
 * 		pointer to the user thread object in TLS
 */
typedef struct
{
	g_user_threadlocal* userThreadLocal;
}__attribute__((packed)) g_syscall_task_get_tls;

/**
 * @field path
 * 		buffer containing the path
 *
 * @field result
 * 		one of the {g_set_working_directory_status} codes
 *
 * @security-level APPLICATION
 */
typedef struct
{
  char* path;

  g_set_working_directory_status result;
}__attribute__((packed)) g_syscall_set_working_directory;

/**
 * @field buffer
 * 		buffer with the given size
 *
 * @field maxlen
 * 		maximum number of bytes to write to the buffer
 *
 * @security-level APPLICATION
 */
typedef struct
{
  char* buffer;
  size_t maxlen;
  g_get_working_directory_status result;
}__attribute__((packed)) g_syscall_get_working_directory;

/**
 * @field buffer
 * 		buffer with a size of at least {G_PATH_MAX} bytes
 *
 * @security-level APPLICATION
 */
typedef struct
{
  char* buffer;
}__attribute__((packed)) g_syscall_get_executable_path;

/**
 * @field target
 * 		yield target task
 *
 * @security-level APPLICATION
 */
typedef struct
{
	g_tid target;
}__attribute__((packed)) g_syscall_yield;

__END_C

#endif
