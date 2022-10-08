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

#ifndef GHOST_GLOBAL_IPC_MESSAGE
#define GHOST_GLOBAL_IPC_MESSAGE

#include "ghost/common.h"
#include "ghost/kernel.h"
#include "ghost/stdint.h"

__BEGIN_C

// message transactions
typedef uint32_t g_message_transaction;
#define G_MESSAGE_TRANSACTION_NONE			0
#define G_MESSAGE_TRANSACTION_FIRST			1

// message header
typedef struct _g_message_header {
	g_tid sender;
	g_message_transaction transaction;
	size_t length;
	struct _g_message_header* previous;
	struct _g_message_header* next;
}__attribute__((packed)) g_message_header;

#define G_MESSAGE_CONTENT(message)			(((uint8_t*) message) + sizeof(g_message_header))

// messaging bounds
#define G_MESSAGE_MAXIMUM_LENGTH			(2048)
#define G_MESSAGE_MAXIMUM_QUEUE_CONTENT		(2048 * 32)

// modes for message sending
typedef int g_message_send_mode;
#define G_MESSAGE_SEND_MODE_BLOCKING ((g_message_send_mode) 0)
#define G_MESSAGE_SEND_MODE_NON_BLOCKING ((g_message_send_mode) 1)

// modes for message receiving
typedef int g_message_receive_mode;
#define G_MESSAGE_RECEIVE_MODE_BLOCKING ((g_message_receive_mode) 0)
#define G_MESSAGE_RECEIVE_MODE_NON_BLOCKING ((g_message_receive_mode) 1)

// status for message sending
typedef int g_message_send_status;
#define G_MESSAGE_SEND_STATUS_SUCCESSFUL ((g_message_send_status) 1)
#define G_MESSAGE_SEND_STATUS_QUEUE_FULL ((g_message_send_status) 2)
#define G_MESSAGE_SEND_STATUS_FAILED ((g_message_send_status) 3)
#define G_MESSAGE_SEND_STATUS_EXCEEDS_MAXIMUM ((g_message_send_status) 4)

// status for message receiving
typedef int g_message_receive_status;
#define G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL ((g_message_receive_status) 1)
#define G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY ((g_message_receive_status) 2)
#define G_MESSAGE_RECEIVE_STATUS_FAILED ((g_message_receive_status) 3)
#define G_MESSAGE_RECEIVE_STATUS_FAILED_NOT_PERMITTED ((g_message_receive_status) 4)
#define G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE ((g_message_receive_status) 5)
#define G_MESSAGE_RECEIVE_STATUS_INTERRUPTED ((g_message_receive_status) 6)

__END_C

#endif
