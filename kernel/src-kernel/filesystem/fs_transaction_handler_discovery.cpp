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

#include "filesystem/fs_transaction_handler_discovery.hpp"
#include "filesystem/fs_delegate.hpp"
#include "filesystem/filesystem.hpp"
#include "ghost/utils/local.hpp"
#include "tasking/wait/waiter_fs_transaction.hpp"
#include "logger/logger.hpp"

/**
 *
 */
g_fs_transaction_handler_start_status g_fs_transaction_handler_discovery::start_transaction(g_thread* thread) {

	// check if this node is already discovered
	g_fs_node* parent = 0;
	g_fs_node* child = 0;
	g_local<char> last_name(new char[G_PATH_MAX]);
	g_filesystem::find_existing(absolute_path, &parent, &child, last_name(), follow_symlinks);

	// if the node already exists, tell the handler that discovery was successful
	if (child) {
		status = G_FS_DISCOVERY_SUCCESSFUL;
		node = child;
		all_nodes_discovered = true;
		finish_transaction(thread, child->get_delegate());
		return G_FS_TRANSACTION_START_IMMEDIATE_FINISH;
	}

	// otherwise, request the driver delegate to discover it and set to sleep
	g_fs_delegate* delegate = parent->get_delegate();
	if (delegate == nullptr) {
		// this is an error error
		if (parent == g_filesystem::get_root()) {
			g_log_warn("%! mountpoint for '%s' does not exist", "filesystem", absolute_path);
		} else {
			g_log_warn("%! discovery of '%s' failed due to missing delegate on node %i", "filesystem", absolute_path, parent->id);
		}
		status = G_FS_DISCOVERY_ERROR;
		all_nodes_discovered = true;
		finish_transaction(thread, 0);
		return G_FS_TRANSACTION_START_FAILED;
	}

	// set the last discovered parent, so that the open command knows the last existing parent
	last_discovered_parent = parent;

	// start the discovery
	g_fs_transaction_id transaction = delegate->request_discovery(thread, parent, last_name(), this);
	thread->wait(new g_waiter_fs_transaction(this, transaction, delegate));
	return G_FS_TRANSACTION_START_WITH_WAITER;
}

/**
 *
 */
g_fs_transaction_handler_finish_status g_fs_transaction_handler_discovery::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {

	// if not all nodes have been discovered yet, we need to do a loop
	if (!all_nodes_discovered) {

		// allow the delegate to finish
		if (delegate) {
			delegate->finish_discovery(thread, this);
		}

		// if discovering the next node was successful, go on with discovering
		if (status == G_FS_DISCOVERY_SUCCESSFUL) {

			// continue discovery
			auto status = start_transaction(thread);

			// check for possible failure
			if (status == G_FS_TRANSACTION_START_FAILED) {
				return G_FS_TRANSACTION_HANDLING_DONE;

			} else {

				// check if all nodes have been discovered now
				if (!all_nodes_discovered) {
					return G_FS_TRANSACTION_HANDLING_REPEAT_WITH_SAME_HANDLER;
				}
			}
		}
	}

	// when everything is done, perform afterwork; might want to keep waiting again
	return after_finish_transaction(thread);
}
