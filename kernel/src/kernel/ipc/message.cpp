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
#include "kernel/utils/hashmap.hpp"

#include "shared/logger/logger.hpp"


static g_hashmap<g_tid, g_message_queue*>* messageQueues = 0;

void messageInitialize()
{
    messageQueues = hashmapCreateNumeric<g_tid, g_message_queue*>(64);
}

void messageRemoveFromQueue(g_message_queue* queue, g_message_header* message)
{
    queue->size -= sizeof(g_message_header) + message->length;

    if(message == queue->head)
        queue->head = message->next;

    if(message == queue->tail)
        queue->tail = message->previous;

    if(message->next)
        message->next->previous = message->previous;

    if(message->previous)
        message->previous->next = message->next;
}

void messageAddToQueueTail(g_message_queue* queue, g_message_header* message)
{
    queue->size += sizeof(g_message_header) + message->length;

    message->next = 0;
    if(!queue->head)
    {
        queue->head = message;
        queue->tail = message;
        message->previous = 0;
        message->next = 0;
        return;
    }

    message->previous = queue->tail;
    queue->tail->next = message;
    queue->tail = message;
}

g_message_send_status messageSend(g_tid sender, g_tid receiver, void* content, uint32_t length, g_message_transaction tx)
{
    if(length > G_MESSAGE_MAXIMUM_LENGTH)
    {
        return G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM;
    }

    auto receiverEntry = hashmapGetEntry(messageQueues, receiver);

    g_message_queue* queue;
    if(receiverEntry)
    {
        queue = receiverEntry->value;
    }
    else {
        queue = (g_message_queue*) heapAllocate(sizeof(g_message_queue));
        queue->size = 0;
        queue->head = 0;
        queue->tail = 0;
        mutexInitialize(&queue->lock);
        hashmapPut(messageQueues, receiver, queue);
    }

    mutexAcquire(&queue->lock);

    uint32_t len = sizeof(g_message_header) + length;
    if(queue->size + len > G_MESSAGE_MAXIMUM_QUEUE_CONTENT)
    {
        mutexRelease(&queue->lock);
        return G_MESSAGE_SEND_STATUS_QUEUE_FULL;
    }

    g_message_header* message = (g_message_header*) heapAllocate(len);
    message->length = length;
    message->sender = sender;
    message->transaction = tx;
    memoryCopy(G_MESSAGE_CONTENT(message), content, length);
	messageAddToQueueTail(queue, message);

    mutexRelease(&queue->lock);

    return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}

g_message_receive_status messageReceive(g_tid receiver, g_message_header* out, uint32_t max, g_message_transaction tx)
{
    auto receiverEntry = hashmapGetEntry(messageQueues, receiver);

    g_message_queue* queue;
    if(receiverEntry)
    {
        queue = receiverEntry->value;
    }
    else {
        return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
    }

    mutexAcquire(&queue->lock);

    g_message_header* message = queue->head;
    while(message)
    {
        if(tx == G_MESSAGE_TRANSACTION_NONE || message->transaction == tx)
        {
            uint32_t len = sizeof(g_message_header) + message->length;
            if(len > max)
            {
                mutexRelease(&queue->lock);
                return G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE;
            }

            memoryCopy((void*) out, message, len);
			messageRemoveFromQueue(queue, message);
			heapFree(message);

            mutexRelease(&queue->lock);
            return G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
        }

        message = message->next;
    }

    mutexRelease(&queue->lock);
    return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
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

