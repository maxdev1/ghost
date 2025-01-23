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

#ifndef GHOST_API_MUTEX
#define GHOST_API_MUTEX

#include "common.h"
#include "mutex/types.h"

__BEGIN_C
/**
 * Creates an mutex used for locking.
 *
 * @returns mutex
 * 		the mutex
 *
 * @security-level APPLICATION
 */
g_user_mutex g_mutex_initialize();
g_user_mutex g_mutex_initialize_r(g_bool reentrant);

/**
 * Acquires the mutex. If the mutex is locked, the executing task must
 * wait until the task that owns the mutex has finished its work and sets
 * it to false.
 *
 * @param mutex
 * 		the mutex to use
 *
 * @security-level APPLICATION
 */
void g_mutex_acquire(g_user_mutex mutex);
g_bool g_mutex_acquire_to(g_user_mutex mutex, uint64_t timeout);

/**
 * Trys acquire the mutex. If the lock is already locked, the function
 * returns 0. Otherwise, the lock is set as in {g_mutex_acquire} and the
 * function returns 1.
 *
 * @param mutex
 * 		the mutex to use
 *
 * @security-level APPLICATION
 */
g_bool g_mutex_try_acquire(g_user_mutex mutex);
g_bool g_mutex_try_acquire_to(g_user_mutex mutex, uint64_t timeout);

/**
 * Unlocks the mutex.
 *
 * @param mutex
 * 		the mutex to use
 *
 * @security-level APPLICATION
 */
void g_mutex_release(g_user_mutex mutex);

/**
 * Frees a mutex when no longer needed.
 *
 * @param mutex
 * 		the mutex to use
 *
 * @security-level APPLICATION
 */
void g_mutex_destroy(g_user_mutex mutex);


__END_C

#endif
