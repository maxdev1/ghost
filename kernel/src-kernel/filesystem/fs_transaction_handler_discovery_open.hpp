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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY_OPEN
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY_OPEN

#include "filesystem/fs_transaction_handler_discovery.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"
#include "logger/logger.hpp"
#include "filesystem/fs_transaction_handler_open.hpp"

/**
 *
 */
class g_fs_transaction_handler_discovery_open: public g_fs_transaction_handler_discovery {
public:
	g_contextual<g_syscall_fs_open*> data;

	/**
	 *
	 */
	g_fs_transaction_handler_discovery_open(char* absolute_path_in, g_contextual<g_syscall_fs_open*> data) :
			g_fs_transaction_handler_discovery(absolute_path_in), data(data) {

	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status after_finish_transaction(g_thread* thread) {

		if (status == G_FS_DISCOVERY_SUCCESSFUL || status == G_FS_DISCOVERY_NOT_FOUND) {
			// create and start the new handler
			g_fs_node* discovered_node = nullptr;
			if (status == G_FS_DISCOVERY_NOT_FOUND) {
				discovered_node = last_discovered_parent;
			} else {
				discovered_node = node;
			}

			g_fs_transaction_handler_open* handler = new g_fs_transaction_handler_open(data, status, discovered_node);
			auto start_status = handler->start_transaction(thread);

			if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
				return G_FS_TRANSACTION_HANDLING_KEEP_WAITING_WITH_NEW_HANDLER;

			} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
				return G_FS_TRANSACTION_HANDLING_DONE;

			} else {
				data()->status = G_FS_OPEN_ERROR;
				g_log_warn("%! failed to start actual open handler after node discovery", "filesystem");
				return G_FS_TRANSACTION_HANDLING_DONE;
			}

		} else if (status == G_FS_DISCOVERY_ERROR || status == G_FS_DISCOVERY_BUSY) {
			data()->fd = -1;
			data()->status = G_FS_OPEN_ERROR;
		}

		return G_FS_TRANSACTION_HANDLING_DONE;
	}

};

#endif
