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

#include "ghost/user.h"

g_bool __g_mutex_acquire(g_user_mutex mutex, bool trying, uint64_t timeout)
{
	g_syscall_user_mutex_acquire data;
	data.mutex = mutex;
	data.trying = trying;
	data.timeout = timeout;

	data.wasSet = false;
	g_syscall(G_SYSCALL_USER_MUTEX_ACQUIRE, (g_address) &data);
	return data.wasSet;
}

void g_mutex_acquire(g_user_mutex mutex)
{
	__g_mutex_acquire(mutex, false, 0);
}

g_bool g_mutex_acquire_to(g_user_mutex mutex, uint64_t timeout)
{
	return __g_mutex_acquire(mutex, false, timeout);
}

g_bool g_mutex_try_acquire(g_user_mutex mutex)
{
	return __g_mutex_acquire(mutex, true, 0);
}

g_bool g_mutex_try_acquire_to(g_user_mutex mutex, uint64_t timeout)
{
	return __g_mutex_acquire(mutex, true, timeout);
}
