/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "message_topics.hpp"

#include <shared/memory/memory.hpp>

#include "shared/utils/string.hpp"
#include "kernel/utils/hashmap_string.hpp"

static g_hashmap<const char*, g_message_topic*>* messageTopics = nullptr;
static g_mutex messageTopicsLock;

g_message_topic* _messageTopicsGetOrCreate(const char* name);
void _messageTopicsAddToTail(g_message_topic* topic, g_message_header* message);

void messageTopicsInitialize()
{
	messageTopics = hashmapCreateString<g_message_topic*>(64);
	mutexInitializeGlobal(&messageTopicsLock);
}

g_message_send_status messageTopicsPost(const char* topicName, g_tid sender, void* content, uint32_t length)
{
	g_message_topic* topic = _messageTopicsGetOrCreate(topicName);
	mutexAcquire(&topic->lock);

	// Put message into the topic
	uint32_t lengthWithHeader = sizeof(g_message_header) + length;
	auto message = (g_message_header*) heapAllocate(lengthWithHeader);
	message->length = length;
	message->sender = sender;
	message->transaction = topic->nextTransaction++;
	memoryCopy(G_MESSAGE_CONTENT(message), content, length);
	_messageTopicsAddToTail(topic, message);
	waitQueueWake(&topic->waitersReceive);

	mutexRelease(&topic->lock);
	return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}

g_message_receive_status messageTopicsReceive(const char* topicName, g_message_transaction startAfter, void* out,
                                              uint32_t max)
{
	auto topic = _messageTopicsGetOrCreate(topicName);
	mutexAcquire(&topic->lock);

	// Find message that has higher or equal transaction number
	auto message = topic->head;
	while(message)
	{
		if(message->transaction > startAfter)
			break;
		message = message->next;
	}

	// Return it
	g_message_receive_status status;
	if(message)
	{
		size_t lengthWithHeader = sizeof(g_message_header) + message->length;
		if(lengthWithHeader > max)
		{
			status = G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE;
		}
		else
		{
			memoryCopy(out, message, lengthWithHeader);
			status = G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
		}
	}
	else
	{
		status = G_MESSAGE_RECEIVE_STATUS_EMPTY;
	}

	mutexRelease(&topic->lock);
	return status;
}

g_message_topic* _messageTopicsGetOrCreate(const char* name)
{
	mutexAcquire(&messageTopicsLock);
	auto entry = hashmapGetEntry(messageTopics, name);

	g_message_topic* topic;
	if(entry)
	{
		topic = entry->value;
	}
	else
	{
		topic = (g_message_topic*) heapAllocate(sizeof(g_message_topic));
		mutexInitializeTask(&topic->lock, __func__);
		topic->name = stringDuplicate(name);
		topic->size = 0;
		topic->head = nullptr;
		topic->tail = nullptr;
		topic->nextTransaction = 0;
		waitQueueInitialize(&topic->waitersReceive);
		hashmapPut(messageTopics, name, topic);
	}

	mutexRelease(&messageTopicsLock);
	return topic;
}

void messageTopicsWaitForReceive(const char* topicName, g_tid receiver)
{
	auto topic = _messageTopicsGetOrCreate(topicName);
	waitQueueAdd(&topic->waitersReceive, receiver);
}

void messageTopicsUnwaitForReceive(const char* topicName, g_tid receiver)
{
	auto topic = _messageTopicsGetOrCreate(topicName);
	waitQueueRemove(&topic->waitersReceive, receiver);
}

void _messageTopicsAddToTail(g_message_topic* topic, g_message_header* message)
{
	mutexAcquire(&topic->lock);

	topic->size += sizeof(g_message_header) + message->length;

	message->next = nullptr;
	if(topic->head)
	{
		message->previous = topic->tail;
		topic->tail->next = message;
		topic->tail = message;
	}
	else
	{
		topic->head = message;
		topic->tail = message;
		message->previous = nullptr;
		message->next = nullptr;
	}

	mutexRelease(&topic->lock);
}
