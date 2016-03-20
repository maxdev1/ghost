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
#include "filesystem/fs_transaction_handler_read_directory.hpp"

#include "logger/logger.hpp"

/**
 *
 */
g_fs_transaction_handler_start_status g_fs_transaction_handler_read_directory::start_transaction(g_thread* thread) {
	return G_FS_TRANSACTION_START_IMMEDIATE_FINISH;
}

/**
 *
 */
g_fs_transaction_handler_finish_status g_fs_transaction_handler_read_directory::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {

	// check if it was called after a refresh, and the status of the refresh was good
	if (causing_handler != nullptr && causing_handler->status != G_FS_DIRECTORY_REFRESH_SUCCESSFUL) {
		data()->status = G_FS_READ_DIRECTORY_ERROR;
		return G_FS_TRANSACTION_HANDLING_DONE;
	}

	// find node at position
	g_fs_node* item = nullptr;

	int position = data()->iterator->position;

	int iterpos = 0;
	auto iter = folder->children;
	while (iter) {
		if (iterpos == position) {
			item = iter->value;
			break;
		}
		iter = iter->next;
		++iterpos;
	}

	// EOD if none found
	if (item == nullptr) {
		data()->status = G_FS_READ_DIRECTORY_EOD;
		return G_FS_TRANSACTION_HANDLING_DONE;
	}

	// copy data to output
	g_memory::copy(data()->iterator->entry_buffer.name, item->name, g_string::length(item->name) + 1);
	++data()->iterator->position;
	data()->iterator->entry_buffer.node_id = item->id;
	data()->iterator->entry_buffer.type = item->type;
	data()->status = G_FS_READ_DIRECTORY_SUCCESSFUL;
	return G_FS_TRANSACTION_HANDLING_DONE;
}
