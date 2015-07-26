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

__BEGIN_C

/**
 * Structure of a message.
 *
 * @field type			the type of message
 * @field sender		id of the sender task, automatically set by the kernel
 * @field topic			the topic/transaction id, used for topic-message-receiving
 * @field parameterX	a parameter
 */
typedef struct {
	uint32_t type;
	uint32_t sender;
	uint32_t topic;

	uint32_t parameterA;
	uint32_t parameterB;
	uint32_t parameterC;
	uint32_t parameterD;
}__attribute__((packed)) g_message;

// creates an empty message
#define g_message_empty(name)	g_message name = { .type = 0, .sender = 0, .topic = 0, .parameterA = 0, .parameterB = 0, .parameterC = 0, .parameterD = 0 };

// modes for message receiving
typedef int g_message_receive_mode;
static const g_message_receive_mode G_MESSAGE_RECEIVE_MODE_BLOCKING = 0;
static const g_message_receive_mode G_MESSAGE_RECEIVE_MODE_NON_BLOCKING = 1;

// status for message sending
typedef int g_message_send_status;
static const g_message_send_status G_MESSAGE_SEND_STATUS_SUCCESSFUL = 1;
static const g_message_send_status G_MESSAGE_SEND_STATUS_QUEUE_FULL = 2;
static const g_message_send_status G_MESSAGE_SEND_STATUS_FAILED = 3;

// status for message receiving
typedef int g_message_receive_status;
static const g_message_receive_status G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL = 1;
static const g_message_receive_status G_MESSAGE_RECEIVE_STATUS_QUEUE_EMPTY = 2;
static const g_message_receive_status G_MESSAGE_RECEIVE_STATUS_FAILED = 3;
static const g_message_receive_status G_MESSAGE_RECEIVE_STATUS_FAILED_NOT_PERMITTED = 4;

__END_C

#endif
