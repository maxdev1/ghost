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
#include "filesystem/fs_transaction_handler_directory_refresh.hpp"
#include "tasking/wait/waiter_fs_transaction.hpp"
#include "logger/logger.hpp"

/**
 *
 */
g_fs_transaction_handler_start_status g_fs_transaction_handler_directory_refresh::start_transaction(g_thread* thread) {

	// take the delegate
	g_fs_delegate* delegate = folder->get_delegate();
	if (delegate == nullptr) {
		data()->status = G_FS_READ_DIRECTORY_ERROR;
		g_log_warn("%! reading directory failed due to missing delegate on node %i", "filesystem", folder->id);
		return G_FS_TRANSACTION_START_FAILED;
	}

	// request the refresh
	g_fs_transaction_id transaction = delegate->request_directory_refresh(thread, folder, this);
	thread->wait(new g_waiter_fs_transaction(this, transaction, delegate));
	return G_FS_TRANSACTION_START_WITH_WAITER;
}

/**
 *
 */
g_fs_transaction_handler_finish_status g_fs_transaction_handler_directory_refresh::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {

	if (delegate) {
		delegate->finish_directory_refresh(thread, this);
	}

	if (unfinished_handler) {
		return unfinished_handler->finish_transaction(thread, delegate);
	}

	return G_FS_TRANSACTION_HANDLING_DONE;
}
