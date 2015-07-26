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

#include "ghost/user.h"

// redirect
g_message_receive_status g_recv_topic_msg(uint32_t taskId, uint32_t ident, g_message* message) {
	return g_recv_topic_msg_m(taskId, ident, message, G_MESSAGE_RECEIVE_MODE_BLOCKING);
}

/**
 *
 */
g_message_receive_status g_recv_topic_msg_m(uint32_t taskId, uint32_t ident, g_message* message, int mode) {
	g_syscall_recv_topic_msg receiveData;
	receiveData.taskId = taskId;
	receiveData.message = message;
	receiveData.topic = ident;
	receiveData.mode = mode;
	g_syscall(G_SYSCALL_MAILBOX_RECEIVE_WITH_IDENT, (uint32_t) &receiveData);
	return receiveData.receiveResult;
}
