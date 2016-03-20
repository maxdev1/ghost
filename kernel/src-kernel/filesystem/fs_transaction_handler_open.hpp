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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_OPEN
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_OPEN

#include "filesystem/fs_transaction_handler.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"

/**
 *
 */
class g_fs_transaction_handler_open: public g_fs_transaction_handler {
public:
	g_contextual<g_syscall_fs_open*> data;
	g_fs_discovery_status discovery_status;
	g_fs_node* node;

	g_fs_open_status status = G_FS_OPEN_ERROR;

	/**
	 * @param data
	 * 		system call data structure
	 * @param discovery_status
	 * 		status of the preceding discovery operation
	 * @param node
	 * 		discovered node to use; must be the actual node if the node was found,
	 * 		otherwise the last discovered parent
	 */
	g_fs_transaction_handler_open(g_contextual<g_syscall_fs_open*> data, g_fs_discovery_status discovery_status, g_fs_node* node) :
			data(data), discovery_status(discovery_status), node(node) {
	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_start_status start_transaction(g_thread* thread);

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status finish_transaction(g_thread* thread, g_fs_delegate* delegate);

};

#endif
