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
#include "filesystem/fs_transaction_handler_open.hpp"
#include "filesystem/filesystem.hpp"
#include "logger/logger.hpp"
#include "tasking/wait/waiter_fs_transaction.hpp"

/**
 *
 */
g_fs_transaction_handler_start_status g_fs_transaction_handler_close::start_transaction(g_thread* thread) {

	// check for the driver delegate
	g_fs_delegate* delegate = node->get_delegate();
	if (delegate == 0) {
		g_log_warn("%! failed to close descriptor %i due to missing delegate", "filesystem", fd);
		return G_FS_TRANSACTION_START_FAILED;
	}

	// start transaction by requesting the delegate
	g_fs_transaction_id transaction = delegate->request_close(thread, this, fd, node);

	// check status for possible immediate finish
	if (g_waiter_fs_transaction::is_transaction_waiting(thread, this, transaction, delegate)) {
		thread->wait(new g_waiter_fs_transaction(this, transaction, delegate));
		return G_FS_TRANSACTION_START_WITH_WAITER;
	}

	return G_FS_TRANSACTION_START_IMMEDIATE_FINISH;
}

/**
 *
 */
g_fs_transaction_handler_finish_status g_fs_transaction_handler_close::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {

	delegate->finish_close(thread, this);

	data()->status = status;

	// once delegate says closing was successful, unmap it
	if (status == G_FS_CLOSE_SUCCESSFUL) {
		if (!g_filesystem::unmap_file(thread->process->main->id, node, fd)) {
			g_log_warn("%! delegate failed to close file descriptor %i in process %i", "filesystem", fd->id, thread->process->main->id);
			status = G_FS_CLOSE_ERROR;
		}
	}

	return G_FS_TRANSACTION_HANDLING_DONE;
}
