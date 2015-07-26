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
 *
 */
class g_fs_transaction_handler_discovery: public g_fs_transaction_handler {
public:
	g_fs_discovery_status status = G_FS_DISCOVERY_ERROR;
	g_fs_node* node = 0;
	char* absolute_path;
	bool all_nodes_discovered = false;

	/**
	 *
	 */
	g_fs_transaction_handler_discovery(char* absolute_path_in) {

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
	virtual g_fs_transaction_handler_status finish_transaction(g_thread* thread, g_fs_delegate* delegate);

	/**
	 *
	 */
	virtual g_fs_transaction_handler_status perform_afterwork(g_thread* thread) = 0;
};

#endif
