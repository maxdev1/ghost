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

#include "filesystem/fs_delegate.hpp"
#include "filesystem/fs_transaction_handler_get_length.hpp"
#include "tasking/wait/waiter_fs_transaction.hpp"

/**
 *
 */
g_fs_transaction_handler_start_status g_fs_transaction_handler_get_length::start_transaction(g_thread* thread) {

	g_fs_delegate* delegate = node->get_delegate();
	if (delegate) {
		g_fs_transaction_id transaction = delegate->request_get_length(thread, node, this);
		thread->wait(new g_waiter_fs_transaction(this, transaction, delegate));
		return G_FS_TRANSACTION_START_WITH_WAITER;
	}

	return G_FS_TRANSACTION_START_FAILED;
}

/**
 *
 */
g_fs_transaction_handler_finish_status g_fs_transaction_handler_get_length::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {
	delegate->finish_get_length(thread, this);

	perform_afterwork(thread);
	return G_FS_TRANSACTION_HANDLING_DONE;
}
