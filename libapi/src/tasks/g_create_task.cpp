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

#include "ghost/syscall.h"
#include "ghost/tasks.h"
#include "ghost/tasks/callstructs.h"

/**
 * Task setup routine, used by the task creation call. Assumes that the created task
 * has a valid "userEntry" - otherwise exits with error code -1.
 */
void __g_task_setup_routine()
{
	g_syscall_get_task_entry data;
	g_syscall(G_SYSCALL_GET_TASK_ENTRY, (g_address) &data);

	void (*userEntry)(void*) = (void(*)(void*)) (data.userEntry);
	if(userEntry)
	{
		(userEntry)(data.userData);
	}

	return g_exit_task();
}

// redirect
g_tid g_create_task(void* function)
{
	return g_create_task_das(function, nullptr, G_TASK_CORE_AFFINITY_NONE, nullptr);
}

// redirect
g_tid g_create_task_d(void* function, void* userData)
{
	return g_create_task_das(function, userData, G_TASK_CORE_AFFINITY_NONE, nullptr);
}

// redirect
g_tid g_create_task_a(void* function, uint8_t coreAffinity)
{
	return g_create_task_das(function, nullptr, coreAffinity, nullptr);
}

// redirect
g_tid g_create_task_da(void* function, void* userData, uint8_t coreAffinity)
{
	return g_create_task_das(function, userData, coreAffinity, nullptr);
}

// redirect
g_tid g_create_task_ds(void* function, void* userData, g_create_task_status* outStatus)
{
	return g_create_task_das(function, userData, G_TASK_CORE_AFFINITY_NONE, outStatus);
}

/**
 *
 */
g_tid g_create_task_das(void* function, void* userData, uint8_t coreAffinity, g_create_task_status* outStatus)
{
	g_syscall_create_task data;
	data.initialEntry = (void*) __g_task_setup_routine;
	data.userEntry = function;
	data.userData = userData;
	data.coreAffinity = coreAffinity;

	g_syscall(G_SYSCALL_CREATE_TASK, (g_address) &data);
	if(outStatus)
		*outStatus = data.status;

	return data.threadId;
}
