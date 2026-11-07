/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef GHOST_API_SYSTEM_POWER
#define GHOST_API_SYSTEM_POWER

#include "../common.h"
#include "../stdint.h"

__BEGIN_C

typedef uint8_t g_power_action;
#define G_POWER_SHUTDOWN 0
#define G_POWER_REBOOT 1

typedef uint8_t g_power_status;
#define G_POWER_SUCCESSFUL 0
#define G_POWER_BUSY 1
#define G_POWER_NOT_SUPPORTED 2
#define G_POWER_CANCELLED 3
#define G_POWER_IO_ERROR 4
#define G_POWER_TIMED_OUT 5
#define G_POWER_HARDWARE_ERROR 6
#define G_POWER_NOT_PERMITTED 7
#define G_POWER_INVALID_ARGUMENT 8

typedef uint8_t g_power_role;
#define G_POWER_APPLICATION 0
#define G_POWER_STORAGE 1
#define G_POWER_PLATFORM 2
#define G_POWER_STORAGE_PRODUCER 3

#define G_POWER_EVENT_MAGIC 0x47505752u
#define G_POWER_EVENT_PREPARE 0
#define G_POWER_EVENT_CANCEL 1

/**
 * Sent by the kernel (sender G_TID_NONE) to registered preparation tasks.
 * Applications may save documents and veto; storage producers stop accepting
 * work and drain accepted operations before filesystem and block flushing.
 * Storage services drain their queues, flush all devices, and block in their
 * successful acknowledgement until rollback.
 */
typedef struct
{
	uint32_t magic;
	uint32_t request;
	g_power_action action;
	uint8_t type;
	int32_t coordinator;
} __attribute__((packed)) g_power_event;

#define G_POWER_CALL_REQUEST 0
#define G_POWER_CALL_REGISTER 1
#define G_POWER_CALL_ACKNOWLEDGE 2

typedef struct
{
	uint32_t request;
	uint8_t operation;
	g_power_action action;
	g_power_role role;
	uint8_t ready;
	g_power_status status;
} __attribute__((packed)) g_syscall_system_power;

__END_C

#endif
