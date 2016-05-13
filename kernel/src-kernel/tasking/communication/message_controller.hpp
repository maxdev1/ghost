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

#ifndef IPC_MAILBOX
#define IPC_MAILBOX

#include "ghost/stdint.h"
#include "ghost/ipc.h"

#define G_MESSAGE_QUEUE_SIZE		32

/**
 *
 */
struct g_message_queue_head {
	g_message_header* first = 0;
	g_message_header* last = 0;
	size_t total = 0;
};

/**
 *
 */
class g_message_controller {
public:
	static void clear(g_tid tid);
	static g_message_send_status send_message(g_tid target, g_tid source, void* message, size_t length, g_message_transaction tx);
	static g_message_receive_status receive_message(g_tid target, g_message_header* out, size_t max, g_message_transaction tx);
};

#endif
