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

struct g_message_queue
{
	g_mutex lock;
	g_message_header* head;
	g_message_header* tail;
	uint32_t size;

	g_tid task;
	g_wait_queue_entry* waitersSend;
};

/**
 * Initializes basic structures required for messaging.
 */
void messageInitialize();

/**
 * Sends a message.
 */
g_message_send_status messageSend(g_tid sender, g_tid receiver, void* content, uint32_t length, g_message_transaction tx);

/**
 * Receives a message.
 */
g_message_receive_status messageReceive(g_tid receiver, g_message_header* out, uint32_t max, g_message_transaction tx);

g_message_transaction messageNextTxId();

/**
 * When a task is removed, this function is called to cleanup any occupied memory.
 */
void messageTaskRemoved(g_tid task);

void messageWaitForSend(g_tid sender, g_tid receiver);
void messageUnwaitForSend(g_tid sender, g_tid receiver);
void messageWaitForReceive(g_tid receiver);
void messageUnwaitForReceive(g_tid receiver);

#endif
