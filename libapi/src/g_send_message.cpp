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
g_message_send_status g_send_message(g_tid tid, void* buf, size_t len) {
	return g_send_message_tm(tid, buf, len, G_MESSAGE_TRANSACTION_NONE, G_MESSAGE_SEND_MODE_BLOCKING);
}

/**
 *
 */
g_message_send_status g_send_message_m(g_tid tid, void* buf, size_t len, g_message_send_mode mode) {
	return g_send_message_tm(tid, buf, len, G_MESSAGE_TRANSACTION_NONE, mode);
}

/**
 *
 */
g_message_send_status g_send_message_t(g_tid tid, void* buf, size_t len, g_message_transaction tx) {
	return g_send_message_tm(tid, buf, len, tx, G_MESSAGE_SEND_MODE_BLOCKING);
}

/**
 *
 */
g_message_send_status g_send_message_tm(g_tid tid, void* buf, size_t len, g_message_transaction tx, g_message_send_mode mode) {
	g_syscall_send_message data;
	data.buffer = buf;
	data.length = len;
	data.receiver = tid;
	data.mode = mode;
	data.transaction = tx;
	g_syscall(G_SYSCALL_MESSAGE_SEND, (uint32_t) &data);
	return data.status;
}

