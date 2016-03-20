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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY_GET_LENGTH
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY_GET_LENGTH

#include "filesystem/fs_transaction_handler_discovery.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"
#include "logger/logger.hpp"

/**
 *
 */
class g_fs_transaction_handler_discovery_get_length: public g_fs_transaction_handler_discovery {
public:
	g_contextual<g_syscall_fs_length*> data;

	/**
	 *
	 */
	g_fs_transaction_handler_discovery_get_length(char* absolute_path_in, bool follow_symlinks, g_contextual<g_syscall_fs_length*> data) :
			g_fs_transaction_handler_discovery(absolute_path_in, follow_symlinks), data(data) {

	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status after_finish_transaction(g_thread* thread) {

		if (status == G_FS_DISCOVERY_SUCCESSFUL) {
			// create and start the new handler
			g_fs_transaction_handler_get_length_default* handler = new g_fs_transaction_handler_get_length_default(data, node);
			auto start_status = handler->start_transaction(thread);

			if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
				return G_FS_TRANSACTION_HANDLING_KEEP_WAITING_WITH_NEW_HANDLER;

			} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
				return G_FS_TRANSACTION_HANDLING_DONE;

			} else {
				data()->status = G_FS_LENGTH_ERROR;
				g_log_warn("%! failed to start get-length handler after node discovery", "filesystem");
				return G_FS_TRANSACTION_HANDLING_DONE;
			}

		} else {
			data()->length = -1;
			data()->status = G_FS_LENGTH_ERROR;
			return G_FS_TRANSACTION_HANDLING_DONE;
		}
	}

};

#endif
