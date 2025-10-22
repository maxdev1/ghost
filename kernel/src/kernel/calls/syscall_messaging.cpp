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
#include "kernel/ipc/message_queues.hpp"
#include "kernel/ipc/message_topics.hpp"
#include "kernel/tasking/user_mutex.hpp"
#include "kernel/logger/logger.hpp"

void syscallMessageSend(g_task* task, g_syscall_send_message* data)
{
	while((data->status = messageQueueSend(task->id, data->receiver, data->buffer, data->length, data->transaction)) ==
	      G_MESSAGE_SEND_STATUS_FULL &&
	      data->mode == G_MESSAGE_SEND_MODE_BLOCKING)
	{
		taskingWait(task, __func__, [task, data]()
		{
			messageQueueWaitForSend(task->id, data->receiver);
		});
	}
	messageQueueUnwaitForSend(task->id, data->receiver);
}

void syscallMessageReceive(g_task* task, g_syscall_receive_message* data)
{
	while((data->status = messageQueueReceive(task->id, data->buffer, data->maximum, data->transaction)) ==
	      G_MESSAGE_RECEIVE_STATUS_EMPTY &&
	      data->mode == G_MESSAGE_RECEIVE_MODE_BLOCKING)
	{
		// TODO: "Break condition" can't work anymore:
		// if(data->break_condition && userMutexAcquire(task, data->break_condition, true, false))
		//{
		//	data->status = G_MESSAGE_RECEIVE_STATUS_INTERRUPTED;
		//	break;
		// }
		taskingWait(task, __func__);
	}
}

void syscallMessageNextTxId(g_task* task, g_syscall_message_next_txid* data)
{
	data->transaction = messageQueueNextTxId();
}

void syscallMessageTopicSend(g_task* task, g_syscall_send_topic_message* data)
{
	while((data->status = messageTopicsPost(data->topic, task->id, data->buffer, data->length)) ==
	      G_MESSAGE_SEND_STATUS_FULL &&
	      data->mode == G_MESSAGE_SEND_MODE_BLOCKING)
	{
		// TODO topics can never go full at the moment
		logInfo("%# bug: message queues should never be full");
		break;
	}
	// TODO unwait
}

void syscallMessageTopicReceive(g_task* task, g_syscall_receive_topic_message* data)
{
	while((data->status = messageTopicsReceive(data->topic, data->start_after, data->buffer, data->maximum)) ==
	      G_MESSAGE_RECEIVE_STATUS_EMPTY &&
	      data->mode == G_MESSAGE_RECEIVE_MODE_BLOCKING)
	{
		taskingWait(task, __func__, [data, task]()
		{
			messageTopicsWaitForReceive(data->topic, task->id);
		});
	}
	messageTopicsUnwaitForReceive(data->topic, task->id);
}
