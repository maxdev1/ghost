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

#include "ghost/syscall.h"
#include "ghost/messages.h"
#include "ghost/messages/callstructs.h"

// redirect
g_message_send_status g_send_topic_message(const char* topic, void* buf, size_t len)
{
	return g_send_topic_message_m(topic, buf, len, G_MESSAGE_SEND_MODE_BLOCKING);
}

/**
 *
 */
g_message_send_status g_send_topic_message_m(const char* topic, void* buf, size_t len, g_message_send_mode mode)
{
	g_syscall_send_topic_message data;
	data.topic = topic;
	data.buffer = buf;
	data.length = len;
	data.mode = mode;

	g_syscall(G_SYSCALL_MESSAGE_TOPIC_SEND, (g_address) &data);

	return data.status;
}
