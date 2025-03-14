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
#include "kernel/ipc/message.hpp"
#include "kernel/tasking/user_mutex.hpp"
#include "kernel/system/interrupts/interrupts.hpp"

void syscallMessageSend(g_task* task, g_syscall_send_message* data)
{
	while((data->status = messageSend(task->id, data->receiver, data->buffer, data->length, data->transaction)) ==
	      G_MESSAGE_SEND_STATUS_QUEUE_FULL &&
	      data->mode == G_MESSAGE_SEND_MODE_BLOCKING)
	{
		INTERRUPTS_PAUSE;
		mutexAcquire(&task->lock);
		task->status = G_TASK_STATUS_WAITING;
		task->waitsFor = "message-send";
		mutexRelease(&task->lock);
		messageWaitForSend(task->id, data->receiver);
		taskingYield();
		INTERRUPTS_RESUME;
	}
	messageUnwaitForSend(task->id, data->receiver);
}

void syscallMessageReceive(g_task* task, g_syscall_receive_message* data)
{
	while((data->status = messageReceive(task->id, data->buffer, data->maximum, data->transaction)) ==
	      G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY &&
	      data->mode == G_MESSAGE_RECEIVE_MODE_BLOCKING)
	{
		/**
		 * TODO: "Break condition" doesn't work anymore since there is no connection between mutexes and
		 * the message wait queues. This must be somehow connected and the task waken when required.
		 */
		// if(data->break_condition && userMutexAcquire(task, data->break_condition, true, false))
		//{
		//	data->status = G_MESSAGE_RECEIVE_STATUS_INTERRUPTED;
		//	break;
		// }

		INTERRUPTS_PAUSE;
		mutexAcquire(&task->lock);
		task->status = G_TASK_STATUS_WAITING;
		task->waitsFor = "message-recv";
		mutexRelease(&task->lock);
		taskingYield();
		INTERRUPTS_RESUME;
	}
}

void syscallMessageNextTxid(g_task* task, g_syscall_message_next_txid* data)
{
	data->transaction = messageNextTxId();
}
