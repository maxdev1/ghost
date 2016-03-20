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
#include "filesystem/fs_transaction_handler_read.hpp"
#include "filesystem/filesystem.hpp"
#include "logger/logger.hpp"
#include "tasking/wait/waiter_fs_transaction.hpp"

/**
 *
 */
g_fs_transaction_handler_start_status g_fs_transaction_handler_read::start_transaction(g_thread* thread) {

	// create a context-bound wrapper for the data buffer
	g_contextual<uint8_t*> bound_buffer(data()->buffer, thread->process->pageDirectory);

	// check for the driver delegate
	g_fs_delegate* delegate = node->get_delegate();
	if (delegate == 0) {
		g_log_warn("%! reading of '%i' failed due to missing delegate on underlying node %i", "filesystem", fd->id, node->id);
		return G_FS_TRANSACTION_START_FAILED;
	}

	// check if the transaction is repeated only
	if (wants_repeat_transaction()) {
		// when a transaction is repeated, the waiter is still on the requesters task
		delegate->request_read(thread, node, data()->length, bound_buffer, fd, this);
		return G_FS_TRANSACTION_START_WITH_WAITER;
	}

	// start transaction by requesting the delegate
	g_fs_transaction_id transaction = delegate->request_read(thread, node, data()->length, bound_buffer, fd, this);

	// check status for possible immediate finish
	if (g_waiter_fs_transaction::is_transaction_waiting(thread, this, transaction, delegate)) {
		// otherwise append waiter
		thread->wait(new g_waiter_fs_transaction(this, transaction, delegate));
		return G_FS_TRANSACTION_START_WITH_WAITER;
	}

	return G_FS_TRANSACTION_START_IMMEDIATE_FINISH;

}

/**
 *
 */
g_fs_transaction_handler_finish_status g_fs_transaction_handler_read::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {
	delegate->finish_read(thread, &status, &result, fd);

	data()->result = result;
	data()->status = status;
	return G_FS_TRANSACTION_HANDLING_DONE;
}
