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

#include "logger/logger.hpp"

/**
 *
 */
g_fs_transaction_handler_status g_fs_transaction_handler_discovery::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {

	// if not all nodes have been discovered yet, we need to do a loop
	if (!all_nodes_discovered) {

		// allow the delegate to finish
		if (delegate) {
			delegate->finish_discovery(thread, this);
		}

		// if discovering the next node was successful, go on with discovering
		if (status == G_FS_DISCOVERY_SUCCESSFUL) {

			bool entire_path_discovered = g_filesystem::discover_absolute_path(thread, absolute_path, this);
			if (!entire_path_discovered) {
				return G_FS_TRANSACTION_HANDLING_KEEP_WAITING;
			}
		}
	}

	// when everything is done, perform afterwork; might want to keep waiting again
	return perform_afterwork(thread);
}
