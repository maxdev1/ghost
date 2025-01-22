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
g_message_receive_status g_receive_message(void* buf, size_t max) {
	return g_receive_message_tmb(buf, max, G_MESSAGE_TRANSACTION_NONE, G_MESSAGE_RECEIVE_MODE_BLOCKING, 0);
}

// redirect
g_message_receive_status g_receive_message_m(void* buf, size_t max, g_message_receive_mode mode) {
	return g_receive_message_tmb(buf, max, G_MESSAGE_TRANSACTION_NONE, mode, 0);
}

// redirect
g_message_receive_status g_receive_message_t(void* buf, size_t max, g_message_transaction tx) {
	return g_receive_message_tmb(buf, max, tx, G_MESSAGE_RECEIVE_MODE_BLOCKING, 0);
}

// redirect
g_message_receive_status g_receive_message_tm(void* buf, size_t max, g_message_transaction tx, g_message_receive_mode mode) {
	return g_receive_message_tmb(buf, max, tx, mode, 0);
}

/**
 *
 */
g_message_receive_status g_receive_message_tmb(void* buf, size_t max, g_message_transaction tx, g_message_receive_mode mode, g_user_mutex break_condition) {

	g_syscall_receive_message data;
	data.buffer = (g_message_header*) buf;
	data.maximum = max;
	data.mode = mode;
	data.transaction = tx;
	data.break_condition = break_condition;
	g_syscall(G_SYSCALL_MESSAGE_RECEIVE, (g_address) &data);
	return data.status;
}
