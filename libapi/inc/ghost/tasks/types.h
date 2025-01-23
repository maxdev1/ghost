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

#ifndef GHOST_API_TASKS_TYPES
#define GHOST_API_TASKS_TYPES

#include "../common.h"
#include "../stdint.h"
#include "../filesystem/types.h"

__BEGIN_C
/**
 * Thread & process ids
 */
typedef int32_t g_tid;
typedef g_tid g_pid;

#define G_TID_NONE ((g_tid) -1)
#define G_PID_NONE ((g_pid) -1)

/**
 * Task execution security levels
 */
typedef uint8_t g_security_level;

#define G_SECURITY_LEVEL_KERNEL 0
#define G_SECURITY_LEVEL_DRIVER 1
#define G_SECURITY_LEVEL_APPLICATION 2

/**
 * Required for thread-local storage. GS contains the index of the segment
 * which points to this structure (System V ABI for x86). With segment-relative
 * addressing, the TLS content is accessed.
 */
typedef struct _g_user_threadlocal
{
    struct _g_user_threadlocal* self;
} g_user_threadlocal;

/**
 * Task types
 */
typedef uint8_t g_task_type;

#define G_TASK_TYPE_DEFAULT ((g_task_type) 0)
#define G_TASK_TYPE_VM86 ((g_task_type) 1)
#define G_TASK_TYPE_VITAL ((g_task_type) 2)

/**
 * Task priority
 */
typedef uint8_t g_task_priority;

#define G_TASK_PRIORITY_NORMAL ((g_task_priority) 0)
#define G_TASK_PRIORITY_IDLE ((g_task_priority) 1)

/**
 * Task setup constants
 */
#define G_TASK_USER_STACK_RESERVED_VIRTUAL_PAGES 16

/**
 * Task states
 */
typedef uint8_t g_task_status;
#define G_TASK_STATUS_DEAD ((g_task_status) 0)
#define G_TASK_STATUS_RUNNING ((g_task_status) 1)
#define G_TASK_STATUS_WAITING ((g_task_status) 2)

// for <g_kill>
typedef uint8_t g_kill_status;
#define G_KILL_STATUS_SUCCESSFUL						((g_kill_status) 0)
#define G_KILL_STATUS_NOT_FOUND							((g_kill_status) 1)
#define G_KILL_STATUS_FAILED					 		((g_kill_status) 2)

// for <g_create_task>
typedef uint8_t g_create_task_status;
#define G_CREATE_TASK_STATUS_SUCCESSFUL				((g_create_task_status) 0)
#define G_CREATE_TASK_STATUS_FAILED					((g_create_task_status) 1)

/**
 * Process information section header
 */
typedef struct _g_object_info
{
    const char* name;

    void (*init)(void);
    void (*fini)(void);
    void (**preinitArray)(void);
    uint32_t preinitArraySize;
    void (**initArray)(void);
    uint32_t initArraySize;
    void (**finiArray)(void);
    uint32_t finiArraySize;
} __attribute__((packed)) g_object_info;

/**
 * The object information structure is used within the process information section
 * to provide details about the process.
 */
typedef struct
{
    /**
     * Information about all loaded ELF objects.
     */
    g_object_info* objectInfos;
    uint32_t objectInfosSize;

    /**
     * Provides a pointer to the "syscall" function of the kernel, required when attempting
     * to use a system call while within a user-space interrupt service routine.
     */
    void (*syscallKernelEntry)(uint32_t, void*);
} __attribute__((packed)) g_process_info;

/**
 * Holds a pointer to the current process information structure. Is initialized by the
 * g_process_get_info system call routine.
 */
extern g_process_info* g_current_process_info;


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
typedef struct
{
    int command;
}__attribute__((packed)) g_spawn_command_header;

typedef struct
{
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

typedef struct
{
    g_spawn_status status;
    g_pid spawned_process_id;
    g_fd stdin_write;
    g_fd stdout_read;
    g_fd stderr_read;
}__attribute__((packed)) g_spawn_command_spawn_response;

// process configuration buffer lengths
#define G_CLIARGS_BUFFER_LENGTH			1024
#define G_CLIARGS_SEPARATOR				0x1F // ASCII character: UNIT SEPARATOR


__END_C

#endif
