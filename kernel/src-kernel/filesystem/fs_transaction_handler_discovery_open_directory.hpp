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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY_OPEN_DIRECTORY
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY_OPEN_DIRECTORY

#include "filesystem/fs_transaction_handler_discovery.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"
#include "logger/logger.hpp"

/**
 *
 */
class g_fs_transaction_handler_discovery_open_directory: public g_fs_transaction_handler_discovery {
public:
	g_contextual<g_syscall_fs_open_directory*> data;

	/**
	 *
	 */
	g_fs_transaction_handler_discovery_open_directory(char* absolute_path_in, g_contextual<g_syscall_fs_open_directory*> data) :
			g_fs_transaction_handler_discovery(absolute_path_in), data(data) {

	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status after_finish_transaction(g_thread* thread) {

		if (status == G_FS_DISCOVERY_SUCCESSFUL) {
			// fill the call iterator with the node id & reset position
			data()->iterator->node_id = node->id;
			data()->iterator->position = 0;
			data()->status = G_FS_OPEN_DIRECTORY_SUCCESSFUL;

		} else if (status == G_FS_DISCOVERY_NOT_FOUND) {
			data()->status = G_FS_OPEN_DIRECTORY_NOT_FOUND;

		} else if (status == G_FS_DISCOVERY_ERROR || status == G_FS_DISCOVERY_BUSY) {
			data()->status = G_FS_OPEN_DIRECTORY_ERROR;
		}

		return G_FS_TRANSACTION_HANDLING_DONE;
	}

};

#endif
