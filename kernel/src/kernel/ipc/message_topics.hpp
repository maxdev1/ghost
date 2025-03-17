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

#ifndef __KERNEL_IPC_MESSAGE_TOPICS__
#define __KERNEL_IPC_MESSAGE_TOPICS__

#include "kernel/utils/wait_queue.hpp"
#include "shared/system/mutex.hpp"
#include <ghost/messages/callstructs.h>

/**
 * A message topic is identified by a name and persists posted message.
 * The transaction is always counted up for each posted message. Receiving tasks
 * must always use the previous transaction number for receiving.
 */
struct g_message_topic
{
    const char* name;
    g_mutex lock;
    g_message_header* head;
    g_message_header* tail;
    uint32_t size;

    g_message_transaction nextTransaction;

    g_wait_queue waitersReceive;
};

/**
* Initializes message topics.
 */
void messageTopicsInitialize();

/**
 * Posts a message to the topic.
 */
g_message_send_status messageTopicsPost(const char* topicName, g_tid sender, void* content, uint32_t length);

/**
 * Receives the next message from the topic, starting at the given transaction index.
 */
g_message_receive_status messageTopicsReceive(const char* topicName, g_message_transaction startAfter, void* out,
                                              uint32_t max);

/**
 * Adds the task to the receive-wait queue of the topic.
 */
void messageTopicsWaitForReceive(const char* topicName, g_tid receiver);

/**
 * Removes the task from the receive-wait queue of the topic.
 */
void messageTopicsUnwaitForReceive(const char* topicName, g_tid receiver);

#endif
