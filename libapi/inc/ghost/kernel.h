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

#ifndef __GHOST_SYS_KERNEL__
#define __GHOST_SYS_KERNEL__

#include "ghost/common.h"
#include "ghost/stdint.h"
#include <stdarg.h>
#include <stddef.h>

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
 * VM86 related
 */
typedef uint8_t g_vm86_call_status;

#define G_VM86_CALL_STATUS_SUCCESSFUL 0
#define G_VM86_CALL_STATUS_FAILED_NOT_PERMITTED 1

typedef struct
{
	uint16_t ax;
	uint16_t bx;
	uint16_t cx;
	uint16_t dx;
	uint16_t si;
	uint16_t di;
	uint16_t ds;
	uint16_t es;
} __attribute__((packed)) g_vm86_registers;

/**
 * Task types
 */
typedef uint8_t g_thread_type;

#define G_THREAD_TYPE_DEFAULT ((g_thread_type) 0)
#define G_THREAD_TYPE_VM86 ((g_thread_type) 1)
#define G_THREAD_TYPE_SYSCALL ((g_thread_type) 2)
#define G_THREAD_TYPE_VITAL ((g_thread_type) 3)

/**
 * Task priority
 */
typedef uint8_t g_thread_priority;

#define G_THREAD_PRIORITY_NORMAL ((g_thread_priority) 0)
#define G_THREAD_PRIORITY_IDLE ((g_thread_priority) 1)

/**
 * Task setup constants
 */
#define G_THREAD_USER_STACK_RESERVED_VIRTUAL_PAGES 16

/**
 * Task states
 */
typedef uint8_t g_thread_status;

#define G_THREAD_STATUS_DEAD ((g_thread_status) 0)
#define G_THREAD_STATUS_RUNNING ((g_thread_status) 1)
#define G_THREAD_STATUS_WAITING ((g_thread_status) 2)

/**
 * Pipes
 */
#define G_PIPE_DEFAULT_CAPACITY 0x400

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

__END_C

#endif
