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
#include "kernel/ipc/message.hpp"


bool waitResolverSleep(g_task* task)
{
	g_wait_resolver_sleep_data* waitData = (g_wait_resolver_sleep_data*) task->waitData;
	return taskingGetLocal()->time > waitData->wakeTime;
}

bool waitResolverAtomicLock(g_task* task)
{
	g_wait_resolver_atomic_lock_data* waitData = (g_wait_resolver_atomic_lock_data*) task->waitData;
	g_syscall_atomic_lock* data = (g_syscall_atomic_lock*) task->syscall.data;

	// check timeout
	if (data->has_timeout && (taskingGetLocal()->time - waitData->startTime > data->timeout)) {
		data->timed_out = true;
		return true;
	}

	// once waiting is finished, set the atom if required
	bool keep_wait = *data->atom_1 && (!data->atom_2 || *data->atom_2);

	if (!keep_wait && data->set_on_finish) {
		*data->atom_1 = true;
		if (data->atom_2) {
			*data->atom_2 = true;
		}
		data->was_set = true;
	}

	return !keep_wait;
}

bool waitResolverJoin(g_task* task)
{
	g_wait_resolver_join_data* waitData = (g_wait_resolver_join_data*) task->waitData;

	g_task* otherTask = taskingGetById(waitData->joinedTaskId);

	/* Other task doesn't exist anymore or is dead, then stop waiting */
	if(otherTask == 0 || otherTask->status == G_THREAD_STATUS_DEAD || otherTask->status == G_THREAD_STATUS_UNUSED)
	{
		return true;
	}

	return false;
}

bool waitResolverSendMessage(g_task* task)
{
	g_syscall_send_message* data = (g_syscall_send_message*) task->syscall.data;

	data->status = messageSend(task->id, data->receiver, data->buffer, data->length, data->transaction);
	if(data->status == G_MESSAGE_SEND_STATUS_QUEUE_FULL)
	{
		return false;
	}
	return true;
}

bool waitResolverReceiveMessage(g_task* task)
{
	g_syscall_receive_message* data = (g_syscall_receive_message*) task->syscall.data;

	if(data->break_condition && *data->break_condition)
	{
		data->status = G_MESSAGE_RECEIVE_STATUS_INTERRUPTED;
		return true;
	}

	data->status = messageReceive(task->id, data->buffer, data->maximum, data->transaction);
	if(data->status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY)
	{
		return false;
	}
	return true;
}

bool waitResolverVm86(g_task* task)
{
	g_wait_vm86_data* waitData = (g_wait_vm86_data*) task->waitData;
	g_syscall_call_vm86* data = (g_syscall_call_vm86*) task->syscall.data;

	g_task* vm86Task = taskingGetById(waitData->vm86TaskId);
	if(vm86Task == 0 || vm86Task->status == G_THREAD_STATUS_DEAD || vm86Task->status == G_THREAD_STATUS_UNUSED)
	{
		/* VM86 task has finished, copy out registers */
		*data->out = *waitData->registerStore;

		heapFree(waitData->registerStore);

		data->status = G_VM86_CALL_STATUS_SUCCESSFUL;
		return true;
	}

	/* VM86 task still working */
	return false;
}
