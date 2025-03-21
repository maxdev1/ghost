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

#include "kernel/calls/syscall_mutex.hpp"
#include "kernel/tasking/user_mutex.hpp"

void syscallMutexInitialize(g_task* task, g_syscall_user_mutex_initialize* data)
{
	data->mutex = userMutexCreate(data->reentrant);
}

void sycallMutexAcquire(g_task* task, g_syscall_user_mutex_acquire* data)
{
	auto status = userMutexAcquire(task, data->mutex, data->timeout, data->trying);
	data->wasSet = status == G_USER_MUTEX_STATUS_ACQUIRED;
	data->hasTimedOut = status == G_USER_MUTEX_STATUS_TIMEOUT;
}

void syscallMutexRelease(g_task* task, g_syscall_user_mutex_release* data)
{
	userMutexRelease(data->mutex);
}

void syscallMutexDestroy(g_task* task, g_syscall_user_mutex_destroy* data)
{
	userMutexDestroy(data->mutex);
}

