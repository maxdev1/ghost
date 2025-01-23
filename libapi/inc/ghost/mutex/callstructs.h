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

#ifndef GHOST_API_MUTEX_CALLSTRUCTS
#define GHOST_API_MUTEX_CALLSTRUCTS

#include "../common.h"
#include "../stdint.h"
#include "types.h"

__BEGIN_C

/**
 * @field mutex
 * 		the mutex
 */
typedef struct
{
	uint8_t reentrant : 1;

	g_user_mutex mutex;
} __attribute__((packed)) g_syscall_user_mutex_initialize;

/**
 * @field mutex
 * 		the mutex
 */
typedef struct
{
	g_user_mutex mutex;
} __attribute__((packed)) g_syscall_user_mutex_destroy;

/**
 * @field mutex
 *      the mutex
 * @property isTry
 */
typedef struct
{
	g_user_mutex mutex;
	uint8_t trying : 1;
	uint64_t timeout;

	uint8_t hasTimedOut : 1;
	uint8_t wasSet : 1;
} __attribute__((packed)) g_syscall_user_mutex_acquire;

/**
 * @field mutex
 * 		the mutex
 */
typedef struct
{
	g_user_mutex mutex;
} __attribute__((packed)) g_syscall_user_mutex_release;

__END_C

#endif
