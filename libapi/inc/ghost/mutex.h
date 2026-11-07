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
 * Creates an unlocked, non-reentrant mutex.
 *
 * Equivalent to g_mutex_initialize_r(0).
 *
 * @return A mutex handle to release with g_mutex_destroy().
 * @security-level APPLICATION
 */
g_user_mutex g_mutex_initialize();

/**
 * Creates an unlocked mutex with optional recursive acquisition.
 *
 * A reentrant mutex requires one release for each successful acquisition.
 *
 * @param[in] reentrant Nonzero to allow the owning thread to acquire the mutex repeatedly.
 * @return A mutex handle to release with g_mutex_destroy().
 * @security-level APPLICATION
 */
g_user_mutex g_mutex_initialize_r(g_bool reentrant);

/**
 * Waits until the mutex is acquired or its handle becomes invalid.
 *
 * Equivalent to g_mutex_acquire_to(mutex, 0), discarding the success result.
 *
 * @param[in] mutex Handle returned by g_mutex_initialize() or g_mutex_initialize_r().
 * @security-level APPLICATION
 */
void g_mutex_acquire(g_user_mutex mutex);

/**
 * Acquires a mutex, waiting until it becomes available or the deadline expires.
 *
 * @param[in] mutex Handle returned by g_mutex_initialize() or g_mutex_initialize_r().
 * @param[in] timeout Timeout in milliseconds; zero waits without a deadline.
 * @return Nonzero if acquired, or zero on timeout or an invalid or destroyed handle.
 * @security-level APPLICATION
 */
g_bool g_mutex_acquire_to(g_user_mutex mutex, uint64_t timeout);

/**
 * Attempts to acquire a mutex without waiting.
 *
 * @param[in] mutex Handle returned by g_mutex_initialize() or g_mutex_initialize_r().
 * @return Nonzero if acquired, or zero if unavailable or invalid.
 * @security-level APPLICATION
 */
g_bool g_mutex_try_acquire(g_user_mutex mutex);

/**
 * Attempts to acquire a mutex once, with an optional deadline.
 *
 * Contention returns immediately; a nonzero timeout does not make this call wait.
 *
 * @param[in] mutex Handle returned by g_mutex_initialize() or g_mutex_initialize_r().
 * @param[in] timeout Deadline offset in milliseconds; zero disables it. The attempt remains
 *                    nonblocking.
 * @return Nonzero if acquired, or zero if unavailable, invalid, or the deadline has expired.
 * @security-level APPLICATION
 */
g_bool g_mutex_try_acquire_to(g_user_mutex mutex, uint64_t timeout);

/**
 * Releases one acquisition of a mutex and wakes waiters when it becomes unlocked.
 *
 * Recursive acquisitions require matching releases.
 *
 * @param[in] mutex Handle returned by g_mutex_initialize() or g_mutex_initialize_r().
 * @security-level APPLICATION
 */
void g_mutex_release(g_user_mutex mutex);

/**
 * Destroys a mutex and wakes its waiting threads.
 *
 * The handle becomes invalid. Coordinate destruction so no caller continues to use it.
 *
 * @param[in] mutex Handle returned by g_mutex_initialize() or g_mutex_initialize_r().
 * @security-level APPLICATION
 */
void g_mutex_destroy(g_user_mutex mutex);


__END_C

#endif
