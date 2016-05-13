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

#include <tasking/communication/message_controller.hpp>
#include <logger/logger.hpp>
#include "utils/hash_map.hpp"
#include "memory/memory.hpp"
#include "memory/collections/address_stack.hpp"

typedef g_hash_map<g_tid, g_message_queue_head*> g_message_queue_map;
static g_message_queue_map* queues = 0;

static g_address_stack g_message_memory_pool_32;
static g_address_stack g_message_memory_pool_64;
static g_address_stack g_message_memory_pool_256;
static g_address_stack g_message_memory_pool_2048;

/**
 *
 */
void refill(g_address_stack* pool, size_t size) {

	for (int i = 0; i < 32; i++) {
		g_address addr = (g_address) new uint8_t[size];
		if (addr == 0) {
			g_log_info("%! out of memory while refilling memory pool %i", "messages", size);
			break;
		}
		pool->push(addr);
	}
}

/**
 *
 */
g_address get_from_pool(g_address_stack* pool, size_t size) {

	g_address addr = pool->pop();

	if (addr == 0) {
		refill(pool, size);
		return get_from_pool(pool, size);
	}

	return addr;
}

/**
 *
 */
g_message_header* get(size_t size) {

	if (size < 32) {
		return (g_message_header*) get_from_pool(&g_message_memory_pool_32, 32);
	} else if (size < 64) {
		return (g_message_header*) get_from_pool(&g_message_memory_pool_64, 64);
	} else if (size < 256) {
		return (g_message_header*) get_from_pool(&g_message_memory_pool_256, 256);
	} else if (size < 2048) {
		return (g_message_header*) get_from_pool(&g_message_memory_pool_2048, 2048);
	}

	g_log_info("%! invalid fill-pool requested, size %i", "messages", size);
	return 0;
}

/**
 *
 */
void release(g_message_header* m) {

	size_t size = sizeof(g_message_header) + m->length;

	if (size < 32) {
		g_message_memory_pool_32.push((g_address) m);
	} else if (size < 64) {
		g_message_memory_pool_64.push((g_address) m);
	} else if (size < 256) {
		g_message_memory_pool_256.push((g_address) m);
	} else if (size < 2048) {
		g_message_memory_pool_2048.push((g_address) m);
	} else {
		g_log_info("%! invalid released requested, size %i", "messages", size);
	}
}

/**
 *
 */
void g_message_controller::clear(g_tid tid) {

	if (queues == 0) {
		return;
	}

	// get the queue
	auto entry = queues->get(tid);
	if (entry == nullptr) {
		return;
	}

	// release all messages
	g_message_queue_head* head = entry->value;
	g_message_header* m = head->first;
	while (m) {
		g_message_header* after = m->next;
		release(m);
		m = after;
	}

	// release the queue
	queues->remove(tid);
	delete head;
}

/**
 *
 */
g_message_send_status g_message_controller::send_message(g_tid target, g_tid source, void* content, size_t content_len, g_message_transaction tx) {

	// ensure queue map
	if (queues == 0) {
		queues = new g_message_queue_map();
	}

	// check if message too long
	if (content_len > G_MESSAGE_MAXIMUM_LENGTH) {
		return G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM;
	}

	// find/create queue head
	g_message_queue_head* queue;

	auto entry = queues->get(target);
	if (entry == nullptr) {
		queue = new g_message_queue_head();
		queues->put(target, queue);
	} else {
		queue = entry->value;
	}

	// check if it exceeds queue maximum
	if (queue->total + content_len > G_MESSAGE_MAXIMUM_QUEUE_CONTENT) {
		return G_MESSAGE_SEND_STATUS_QUEUE_FULL;
	}

	// create message
	g_message_header* message = get(sizeof(g_message_header) + content_len);
	if (message == 0) {
		return G_MESSAGE_RECEIVE_STATUS_FAILED;
	}
	message->transaction = tx;
	message->sender = source;
	message->length = content_len;
	g_memory::copy(G_MESSAGE_CONTENT(message), content, content_len);

	// append to queue
	if (queue->first == nullptr) {
		queue->first = message;
	}
	if (queue->last) {
		queue->last->next = message;
	}
	message->previous = queue->last;
	message->next = nullptr;
	queue->last = message;

	// increment queue total content length
	queue->total += content_len;

	return G_MESSAGE_SEND_STATUS_SUCCESSFUL;
}

/**
 *
 */
g_message_receive_status g_message_controller::receive_message(g_tid target, g_message_header* out, size_t max, g_message_transaction tx) {

	// check for map
	if (queues == 0) {
		return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
	}

	// find queue head
	g_message_queue_head* queue;

	auto entry = queues->get(target);
	if (entry == nullptr) {
		return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
	} else {
		queue = entry->value;
	}

	if (queue == nullptr) {
		return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
	}

	// find message
	g_message_header* message = nullptr;

	if (tx == G_MESSAGE_TRANSACTION_NONE) {
		message = queue->first;

	} else {
		g_message_header* n = queue->first;
		while (n) {
			if (n->transaction == tx) {
				message = n;
				break;
			}
			n = n->next;
		}
	}

	// no message?
	if (message == nullptr) {
		return G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY;
	}

	// check if message exceeds bounds
	size_t content_len = message->length;
	if ((sizeof(g_message_header) + content_len) > max) {
		return G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE;
	}

	// copy message
	g_memory::copy(out, message, sizeof(g_message_header) + content_len);

	// remove from queue
	if (message->next) {
		message->next->previous = message->previous;
	} else {
		queue->last = message->previous;
		if (queue->last) {
			queue->last->next = 0;
		}
	}

	if (message->previous) {
		message->previous->next = message->next;
	} else {
		queue->first = message->next;
		if (queue->first) {
			queue->first->previous = 0;
		}
	}

	// free the message
	release(message);

	// shrink queue total content length
	queue->total -= content_len;

	return G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL;
}

