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

#include <stdarg.h>
#include <stddef.h>
#include "ghost/common.h"
#include "ghost/stdint.h"

__BEGIN_C

/**
 * Type of a process creation identifier
 */
typedef void* g_process_creation_identifier;

/**
 * Thread & process ids
 */
typedef int32_t g_tid;
typedef g_tid g_pid;

/**
 * Task execution security levels
 */
typedef uint8_t g_security_level;

#define G_SECURITY_LEVEL_KERNEL			0
#define G_SECURITY_LEVEL_DRIVER			1
#define G_SECURITY_LEVEL_APPLICATION	2

/**
 * Required by the System V ABI for x86 to have thread-local-storage.
 */
typedef struct _g_user_thread {
	struct _g_user_thread* self;
} g_user_thread;

/**
 * VM86 related
 */
typedef uint8_t g_vm86_call_status;

#define G_VM86_CALL_STATUS_SUCCESSFUL				0
#define G_VM86_CALL_STATUS_FAILED_NOT_PERMITTED		1

typedef struct {
	uint16_t ax;
	uint16_t bx;
	uint16_t cx;
	uint16_t dx;
	uint16_t si;
	uint16_t di;
	uint16_t ds;
	uint16_t es;
}__attribute__((packed)) g_vm86_registers;

/**
 * Task types
 */
typedef uint8_t g_thread_type;

static const g_thread_type G_THREAD_TYPE_MAIN = 0;
static const g_thread_type G_THREAD_TYPE_SUB = 1;
static const g_thread_type G_THREAD_TYPE_VM86 = 2;

/**
 * Task priority
 */
typedef uint8_t g_thread_priority;

static const g_thread_priority G_THREAD_PRIORITY_NORMAL = 0;
static const g_thread_priority G_THREAD_PRIORITY_IDLE = 1;

/**
 * Task setup constants
 */
#define G_THREAD_USER_STACK_RESERVED_VIRTUAL_PAGES		16

/**
 * Pipes
 */
#define G_PIPE_DEFAULT_CAPACITY		0x400

__END_C

#endif
