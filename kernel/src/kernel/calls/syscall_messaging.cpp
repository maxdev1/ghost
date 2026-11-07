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
#include "kernel/logger/logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/memory_user.hpp"
#include "kernel/tasking/user_mutex.hpp"

void syscallMessageSend(g_task* task, g_syscall_send_message* data)
{
	uint8_t bytes[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	data->status = G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM;
	if(data->length > sizeof(bytes))
		return;

	data->status = G_MESSAGE_SEND_STATUS_FAILED;
	if(!memoryCopyFromUser(task, bytes, (g_address) data->buffer, data->length))
		return;

	while((data->status = messageQueueSend(task->id, data->receiver, bytes, data->length, data->transaction)) ==
				  G_MESSAGE_SEND_STATUS_FULL &&
		  data->mode == G_MESSAGE_SEND_MODE_BLOCKING)
	{
		taskingWaitIf([&]() { return messageQueueWaitForSend(task, data->receiver, data->length); });
	}
	messageQueueUnwaitForSend(task->id, data->receiver);
}

void syscallMessageReceive(g_task* task, g_syscall_receive_message* data)
{
	alignas(g_message_header) uint8_t bytes[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	auto count = data->maximum < sizeof(bytes) ? data->maximum : sizeof(bytes);
	data->status = G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE;
	auto destination = memoryUserAcquire(task, (g_address) data->buffer, count, true);
	if(!destination)
		return;

	auto message = (g_message_header*) bytes;
	while((data->status = messageQueueReceive(task->id, message, count, data->transaction)) ==
				  G_MESSAGE_RECEIVE_STATUS_EMPTY &&
		  data->mode == G_MESSAGE_RECEIVE_MODE_BLOCKING)
	{
		taskingWaitIf([&]() { return messageQueueWaitForReceive(task, data->transaction); });
	}

	if(data->status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		message->previous = nullptr;
		message->next = nullptr;
		memoryUserWrite(destination, bytes, sizeof(g_message_header) + message->length);
	}
}

void syscallMessageNextTxId(g_task* task, g_syscall_message_next_txid* data)
{
	data->transaction = messageQueueNextTxId();
}

void syscallMessageTopicSend(g_task* task, g_syscall_send_topic_message* data)
{
	uint8_t bytes[G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	data->status = G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM;
	if(data->length > sizeof(bytes))
		return;

	data->status = G_MESSAGE_SEND_STATUS_FAILED;
	if(!memoryCopyFromUser(task, bytes, (g_address) data->buffer, data->length))
		return;

	char topic[512];
	if(!memoryCopyUserString(task, data->topic, topic, sizeof(topic)))
		return;

	while((data->status = messageTopicsPost(topic, task->id, bytes, data->length)) == G_MESSAGE_SEND_STATUS_FULL &&
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
	char topic[512];
	data->status = G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE;
	if(!memoryCopyUserString(task, data->topic, topic, sizeof(topic)))
		return;

	alignas(g_message_header) uint8_t bytes[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	auto count = data->maximum < sizeof(bytes) ? data->maximum : sizeof(bytes);
	auto destination = memoryUserAcquire(task, (g_address) data->buffer, count, true);
	if(!destination)
		return;

	auto message = (g_message_header*) bytes;
	while((data->status = messageTopicsReceive(topic, data->start_after, message, count)) ==
				  G_MESSAGE_RECEIVE_STATUS_EMPTY &&
		  data->mode == G_MESSAGE_RECEIVE_MODE_BLOCKING)
	{
		taskingWaitIf([&]() { return messageTopicsWaitForReceive(topic, task, data->start_after); });
	}
	messageTopicsUnwaitForReceive(topic, task->id);

	if(data->status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		message->previous = nullptr;
		message->next = nullptr;
		memoryUserWrite(destination, bytes, sizeof(g_message_header) + message->length);
	}
}
