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
#include "__internal.h"

/**
 * Thread setup routine, used by the thread creation call. Assumes that the created thread
 * has a valid "userEntry" - otherwise exits with error code -1.
 */
void threadsetuproutine() {
	g_syscall_get_thread_entry data;
	g_syscall(G_SYSCALL_GET_THREAD_ENTRY, (uint32_t) &data);

	void (*userEntry)(void*) = (void(*)(void*)) (data.userEntry);

	if (userEntry) {
		(userEntry)(data.userData);
	}

	return g_exit(0);
}

// redirect
g_tid g_create_thread(void* function) {
	return g_create_thread_d(function, 0);
}

// redirect
g_tid g_create_thread_d(void* function, void* userData) {
	return g_create_thread_ds(function, userData, 0);
}

/**
 *
 */
g_tid g_create_thread_ds(void* function, void* userData, g_create_thread_status* out_status) {
	g_syscall_create_thread data;
	data.initialEntry = (void*) threadsetuproutine;
	data.userEntry = function;
	data.userData = userData;
	g_syscall(G_SYSCALL_CREATE_THREAD, (uint32_t) &data);
	if (out_status) {
		*out_status = data.status;
	}
	return data.threadId;
}
