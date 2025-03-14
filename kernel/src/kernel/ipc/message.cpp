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

#include "kernel/ipc/message.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/utils/hashmap.hpp"

#include "shared/logger/logger.hpp"

static g_message_transaction messageNextTx = G_MESSAGE_TRANSACTION_FIRST;
static g_mutex messageTxLock;

static g_hashmap<g_tid, g_message_queue*>* messageQueues = nullptr;

void _messageRemoveFromQueue(g_message_queue* queue, g_message_header* message);
void _messageAddToQueueTail(g_message_queue* queue, g_message_header* message);
void _messageWakeWaitingReceiver(g_message_queue* queue);
g_message_queue* _messageGetQueue(g_tid task);

void messageInitialize()
{
	messageQueues = hashmapCreateNumeric<g_tid, g_message_queue*>(64);
	mutexInitializeTask(&messageTxLock);
}

g_message_send_status messageSend(g_tid sender, g_tid receiver, void* content, uint32_t length,
                                  g_message_transaction tx)
{
	if(length > G_MESSAGE_MAXIMUM_LENGTH)
		return G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM;

	g_message_send_status status;
	uint32_t len = sizeof(g_message_header) + length;

	auto queue = _messageGetQueue(receiver);
	mutexAcquire(&queue->lock);
	bool queueFull = queue->size + len > G_MESSAGE_MAXIMUM_QUEUE_CONTENT;
	mutexRelease(&queue->lock);

	if(queueFull)
	{
		status = G_MESSAGE_SEND_STATUS_QUEUE_FULL;
	}
	else
	{
		g_message_header* message = (g_message_header*) heapAllocate(len);
		message->length = length;
		message->sender = sender;
		message->transaction = tx;
		memoryCopy(G_MESSAGE_CONTENT(message), content, length);
		_messageAddToQueueTail(queue, message);
		_messageWakeWaitingReceiver(queue);
		status = G_MESSAGE_SEND_STATUS_SUCCESSFUL;
	}

	return status;
}

g_message_receive_status messageReceive(g_tid receiver, g_message_header* out, uint32_t max, g_message_transaction tx)
{
	auto receiverEntry = hashmapGetEntry(messageQueues, receiver);

	g_message_queue* queue;
	if(receiverEntry)
	{
		queue = receiverEntry->value;
	}
	else
	{
		return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
	}

	mutexAcquire(&queue->lock);
	g_message_header* message = queue->head;
	while(message)
	{
		if(tx == G_MESSAGE_TRANSACTION_NONE || message->transaction == tx)
			break;

		message = message->next;
	}
	mutexRelease(&queue->lock);

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
			_messageRemoveFromQueue(queue, message);
			heapFree(message);
			waitQueueWake(&queue->waitersSend);
			status = G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
		}
	}
	else
	{
		status = G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
	}

	return status;
}

g_message_transaction messageNextTxId()
{
	mutexAcquire(&messageTxLock);
	g_message_transaction tx = messageNextTx++;
	mutexRelease(&messageTxLock);
	return tx;
}


void messageTaskRemoved(g_tid task)
{
	auto receiverEntry = hashmapGetEntry(messageQueues, task);
	if(!receiverEntry)
		return;

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

void messageWaitForSend(g_tid sender, g_tid receiver)
{
	g_message_queue* queue = _messageGetQueue(receiver);
	waitQueueAdd(&queue->waitersSend, sender);
}

void messageUnwaitForSend(g_tid sender, g_tid receiver)
{
	g_message_queue* queue = _messageGetQueue(receiver);
	waitQueueRemove(&queue->waitersSend, sender);
}

void _messageWakeWaitingReceiver(g_message_queue* queue)
{
	g_task* task = taskingGetById(queue->task);
	taskingWake(task);
}

void _messageRemoveFromQueue(g_message_queue* queue, g_message_header* message)
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

void _messageAddToQueueTail(g_message_queue* queue, g_message_header* message)
{
	mutexAcquire(&queue->lock);

	queue->size += sizeof(g_message_header) + message->length;

	message->next = 0;
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
		message->previous = 0;
		message->next = 0;
	}

	mutexRelease(&queue->lock);
}

g_message_queue* _messageGetQueue(g_tid receiver)
{
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
	return queue;
}
