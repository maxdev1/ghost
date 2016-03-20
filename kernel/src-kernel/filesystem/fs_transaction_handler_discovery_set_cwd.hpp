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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY_SET_CWD
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_DISCOVERY_SET_CWD

#include "filesystem/fs_transaction_handler_discovery.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"
#include "ghost/utils/local.hpp"
#include "logger/logger.hpp"

/**
 *
 */
class g_fs_transaction_handler_discovery_set_cwd: public g_fs_transaction_handler_discovery {
public:
	g_contextual<g_syscall_fs_set_working_directory*> data;
	g_thread* unspawned_target;

	/**
	 * @param unspawned_target
	 * 		when an unspawned target is supplied, the working directory is set to
	 * 		the unspawned target process instead of the waiting process.
	 */
	g_fs_transaction_handler_discovery_set_cwd(char* absolute_path_in, g_contextual<g_syscall_fs_set_working_directory*> data, g_thread* unspawned_target) :
			g_fs_transaction_handler_discovery(absolute_path_in), data(data), unspawned_target(unspawned_target) {

	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status after_finish_transaction(g_thread* task) {

		if (status == G_FS_DISCOVERY_SUCCESSFUL) {

			if (!(node->type == G_FS_NODE_TYPE_PIPE || node->type == G_FS_NODE_TYPE_FILE)) {
				g_local<char> node_realpath(new char[G_PATH_MAX]);
				g_filesystem::get_real_path_to_node(node, node_realpath());

				g_thread* target = (unspawned_target != nullptr ? unspawned_target : task);
				g_string::copy(target->process->workingDirectory, node_realpath());
				data()->result = G_SET_WORKING_DIRECTORY_SUCCESSFUL;
				g_log_debug("%! cwd of process %i is now '%s'", "filesystem", target->process->main->id, target->process->workingDirectory);

			} else {
				data()->result = G_SET_WORKING_DIRECTORY_NOT_A_FOLDER;
				g_log_info("%! could not set current working directory to '%s', not a folder", "filesystem", data()->path);
			}

		} else if (status == G_FS_DISCOVERY_NOT_FOUND) {
			data()->result = G_SET_WORKING_DIRECTORY_NOT_FOUND;
			g_log_info("%! could not set current working directory to '%s', node not found", "filesystem", data()->path);

		} else if (status == G_FS_DISCOVERY_ERROR) {
			data()->result = G_SET_WORKING_DIRECTORY_ERROR;
			g_log_info("%! could not set current working directory to '%s', missing delegate on way", "filesystem", data()->path);

		} else if (status == G_FS_DISCOVERY_BUSY) {
			data()->result = G_SET_WORKING_DIRECTORY_ERROR;
			g_log_info("%! could not set current working directory to '%s', delegate busy", "filesystem", data()->path);

		}

		return G_FS_TRANSACTION_HANDLING_DONE;
	}

};

#endif
