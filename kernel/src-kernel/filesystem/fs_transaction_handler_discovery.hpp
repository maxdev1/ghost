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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY

#include "filesystem/fs_transaction_handler.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"
#include "utils/string.hpp"

/**
 * This handler discovers (and creates virtual nodes for) the given absolute path.
 *
 * This is done by looking up all path elements as virtual nodes top-down.
 * If all of the path elements already exist as virtual nodes, this function immediately
 * returns true. Otherwise, the delegate is asked to discover and a {g_waiter_fs_discovery}
 * is appended to the task. This waiter waits for the discovery transaction to finish.
 *
 * The discovery process is called by the waiter itself repeatedly until all path elements
 * are successfully discovered or discovery fails at some point.
 */
class g_fs_transaction_handler_discovery: public g_fs_transaction_handler {
public:
	bool follow_symlinks;
	g_fs_discovery_status status = G_FS_DISCOVERY_ERROR;
	g_fs_node* node = 0;
	g_fs_node* last_discovered_parent = 0;

	char* absolute_path;
	bool all_nodes_discovered = false;

	/**
	 *
	 */
	g_fs_transaction_handler_discovery(char* absolute_path_in, bool follow_symlinks = true) :
			follow_symlinks(follow_symlinks) {

		// clone incoming path
		int len_abs = g_string::length(absolute_path_in);
		absolute_path = new char[len_abs + 1];
		g_string::copy(absolute_path, absolute_path_in);
	}

	/**
	 *
	 */
	~g_fs_transaction_handler_discovery() {
		delete absolute_path;
	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_start_status start_transaction(g_thread* thread);

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status finish_transaction(g_thread* thread, g_fs_delegate* delegate);

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status after_finish_transaction(g_thread* thread) = 0;
};

#endif
