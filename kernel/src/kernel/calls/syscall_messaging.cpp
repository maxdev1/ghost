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

#include "kernel/calls/syscall_messaging.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/ipc/message.hpp"
#include "kernel/tasking/wait.hpp"

void syscallRegisterTaskIdentifier(g_task* task, g_syscall_task_id_register* data)
{
	data->successful = taskingDirectoryRegister(data->identifier, task->id, task->securityLevel);
}

void syscallGetTaskForIdentifier(g_task* task, g_syscall_task_id_get* data)
{
	data->resultTaskId = taskingDirectoryGet(data->identifier);
}

void syscallMessageSend(g_task* task, g_syscall_send_message* data)
{
	data->status = messageSend(task->id, data->receiver, data->buffer, data->length, data->transaction);

	if(data->mode == G_MESSAGE_SEND_MODE_BLOCKING && data->status == G_MESSAGE_SEND_STATUS_QUEUE_FULL)
	{
		waitForMessageSend(task);
		taskingSchedule();
	}
}

void syscallMessageReceive(g_task* task, g_syscall_receive_message* data)
{
	data->status = messageReceive(task->id, data->buffer, data->maximum, data->transaction);

	if(data->mode == G_MESSAGE_RECEIVE_MODE_BLOCKING && data->status == G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY)
	{
		waitForMessageReceive(task);
		taskingSchedule();
	}
}
