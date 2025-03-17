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

#include "kernel/ipc/message_queues.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/utils/hashmap.hpp"

#include "shared/logger/logger.hpp"

static g_message_transaction messageNextTx = G_MESSAGE_QUEUE_TRANSACTION_START;
static g_mutex messageTxLock;

static g_hashmap<g_tid, g_message_queue*>* messageQueues = nullptr;
static g_mutex messageQueuesLock;

void _messageQueuesRemove(g_message_queue* queue, g_message_header* message);
void _messageQueuesAddToTail(g_message_queue* queue, g_message_header* message);
void _messageQueuesWakeWaitingReceiver(g_message_queue* queue);
g_message_queue* _messageQueuesGetOrCreate(g_tid receiver);

void messageQueuesInitialize()
{
	messageQueues = hashmapCreateNumeric<g_tid, g_message_queue*>(64);
	mutexInitializeTask(&messageTxLock);
	mutexInitializeGlobal(&messageQueuesLock);
}

g_message_send_status messageQueueSend(g_tid sender, g_tid receiver, void* content, uint32_t length,
                                       g_message_transaction tx)
{
	if(length > G_MESSAGE_MAXIMUM_MESSAGE_LENGTH)
		return G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM;

	g_message_send_status status;
	uint32_t lengthWithHeader = sizeof(g_message_header) + length;

	auto queue = _messageQueuesGetOrCreate(receiver);
	mutexAcquire(&queue->lock);
	bool queueFull = queue->size + lengthWithHeader > G_MESSAGE_MAXIMUM_QUEUE_CONTENT;
	mutexRelease(&queue->lock);

	if(queueFull)
	{
		status = G_MESSAGE_SEND_STATUS_FULL;
	}
	else
	{
		auto message = (g_message_header*) heapAllocate(lengthWithHeader);
		message->length = length;
		message->sender = sender;
		message->transaction = tx;
		memoryCopy(G_MESSAGE_CONTENT(message), content, length);
		_messageQueuesAddToTail(queue, message);
		_messageQueuesWakeWaitingReceiver(queue);
		status = G_MESSAGE_SEND_STATUS_SUCCESSFUL;
	}

	return status;
}

g_message_receive_status messageQueueReceive(g_tid receiver, g_message_header* out, uint32_t max,
                                             g_message_transaction tx)
{
	auto receiverEntry = hashmapGetEntry(messageQueues, receiver);

	g_message_queue* queue;
	if(receiverEntry)
		queue = receiverEntry->value;
	else
		return G_MESSAGE_RECEIVE_STATUS_EMPTY;

	mutexAcquire(&queue->lock);
	g_message_header* message = queue->head;
	while(message)
	{
		if(tx == G_MESSAGE_TRANSACTION_NONE || message->transaction == tx)
			break;

		message = message->next;
	}

	g_message_receive_status status;
	if(message)
	{
		int32_t len = sizeof(g_message_header) + message->length;
		if(len > max)
		{
			status = G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE;
		}
		else
		{
			memoryCopy(out, message, len);
			_messageQueuesRemove(queue, message);
			heapFree(message);
			waitQueueWake(&queue->waitersSend);
			status = G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
		}
	}
	else
	{
		status = G_MESSAGE_RECEIVE_STATUS_EMPTY;
	}

	mutexRelease(&queue->lock);

	return status;
}

g_message_transaction messageQueueNextTxId()
{
	mutexAcquire(&messageTxLock);
	g_message_transaction tx = messageNextTx++;
	mutexRelease(&messageTxLock);
	return tx;
}


void messageQueueTaskRemoved(g_tid task)
{
	mutexAcquire(&messageQueuesLock);

	auto receiverEntry = hashmapGetEntry(messageQueues, task);
	if(receiverEntry)
	{
		g_message_queue* queue = receiverEntry->value;
		mutexAcquire(&queue->lock);

		g_message_header* head = queue->head;
		while(head)
		{
			g_message_header* next = head->next;
			heapFree(head);
			head = next;
		}

		mutexRelease(&queue->lock);

		hashmapRemove(messageQueues, task);
		heapFree(queue);
	}

	mutexRelease(&messageQueuesLock);
}

void messageQueueWaitForSend(g_tid sender, g_tid receiver)
{
	g_message_queue* queue = _messageQueuesGetOrCreate(receiver);
	waitQueueAdd(&queue->waitersSend, sender);
}

void messageQueueUnwaitForSend(g_tid sender, g_tid receiver)
{
	g_message_queue* queue = _messageQueuesGetOrCreate(receiver);
	waitQueueRemove(&queue->waitersSend, sender);
}

void _messageQueuesWakeWaitingReceiver(g_message_queue* queue)
{
	g_task* task = taskingGetById(queue->task);
	taskingWake(task);
}

void _messageQueuesRemove(g_message_queue* queue, g_message_header* message)
{
	mutexAcquire(&queue->lock);

	queue->size -= sizeof(g_message_header) + message->length;

	if(message == queue->head)
		queue->head = message->next;

	if(message == queue->tail)
		queue->tail = message->previous;

	if(message->next)
		message->next->previous = message->previous;

	if(message->previous)
		message->previous->next = message->next;

	mutexRelease(&queue->lock);
}

void _messageQueuesAddToTail(g_message_queue* queue, g_message_header* message)
{
	mutexAcquire(&queue->lock);

	queue->size += sizeof(g_message_header) + message->length;

	message->next = nullptr;
	if(queue->head)
	{
		message->previous = queue->tail;
		queue->tail->next = message;
		queue->tail = message;
	}
	else
	{
		queue->head = message;
		queue->tail = message;
		message->previous = nullptr;
		message->next = nullptr;
	}

	mutexRelease(&queue->lock);
}

g_message_queue* _messageQueuesGetOrCreate(g_tid receiver)
{
	mutexAcquire(&messageQueuesLock);

	auto entry = hashmapGetEntry(messageQueues, receiver);

	g_message_queue* queue;
	if(entry)
	{
		queue = entry->value;
	}
	else
	{
		queue = (g_message_queue*) heapAllocate(sizeof(g_message_queue));
		mutexInitializeTask(&queue->lock, __func__);
		queue->task = receiver;
		queue->size = 0;
		queue->head = nullptr;
		queue->tail = nullptr;
		waitQueueInitialize(&queue->waitersSend);
		hashmapPut(messageQueues, receiver, queue);
	}

	mutexRelease(&messageQueuesLock);
	return queue;
}
