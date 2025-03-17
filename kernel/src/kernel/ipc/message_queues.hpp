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

#ifndef __KERNEL_IPC_MESSAGE__
#define __KERNEL_IPC_MESSAGE__

#include "kernel/utils/wait_queue.hpp"
#include "shared/system/mutex.hpp"

#include <ghost/messages/callstructs.h>

/**
 * A message queue exists per task and removes messages once they are read by
 * the receiving task.
 */
struct g_message_queue
{
    g_mutex lock;
    g_message_header* head;
    g_message_header* tail;
    uint32_t size;

    g_tid task;
    g_wait_queue waitersSend;
};

/**
 * Initializes the messaging system.
 */
void messageQueuesInitialize();

/**
 * Sends a message.
 */
g_message_send_status messageQueueSend(g_tid sender, g_tid receiver, void* content, uint32_t length,
                                  g_message_transaction tx);

/**
 * Receives a message.
 */
g_message_receive_status messageQueueReceive(g_tid receiver, g_message_header* out, uint32_t max, g_message_transaction tx);

/**
 * @return the next free message transaction ID in the system
 */
g_message_transaction messageQueueNextTxId();

/**
 * Cleans up messages when a task is removed.
 */
void messageQueueTaskRemoved(g_tid task);

/**
 * Adds the sender to the list of waiters that wait for free space in the receivers queue.
 */
void messageQueueWaitForSend(g_tid sender, g_tid receiver);

/**
 * Removes the sender from the send-wait queue.
 */
void messageQueueUnwaitForSend(g_tid sender, g_tid receiver);

/**
 * Adds the receiver to the list of waiters that wait for new messages in the receivers queue.
 */
void messageQueueWaitForReceive(g_tid receiver);

/**
 * Removes the receiver from the receive-wait queue.
 */
void messageQueueUnwaitForReceive(g_tid receiver);

#endif
