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

#include "ghost/calls/calls.h"

#include "kernel/tasking/wait_resolver.hpp"

#include "kernel/memory/heap.hpp"
#include "shared/logger/logger.hpp"

bool waitResolverSleep(g_task* task)
{
	g_wait_resolver_sleep_data* waitData = (g_wait_resolver_sleep_data*) task->waitData;
	return taskingGetLocal()->time > waitData->wakeTime;
}

bool waitResolverAtomicLock(g_task* task)
{
	g_wait_resolver_atomic_lock_data* waitData = (g_wait_resolver_atomic_lock_data*) task->waitData;
	g_syscall_atomic_lock* data = (g_syscall_atomic_lock*) task->syscall.data;

	// check timeout first
	if(data->has_timeout && (taskingGetLocal()->time - waitData->startTime > data->timeout))
	{
		// timeout exceeded
		data->timed_out = true;
		return false;
	}

	// once waiting is finished, set the atom if required
	bool keep_wait = *data->atom_1 && (!data->atom_2 || *data->atom_2);

	if(!keep_wait && data->set_on_finish)
	{
		*data->atom_1 = true;
		if(data->atom_2)
		{
			*data->atom_2 = true;
		}
		data->was_set = true;
	}

	return keep_wait;
}

