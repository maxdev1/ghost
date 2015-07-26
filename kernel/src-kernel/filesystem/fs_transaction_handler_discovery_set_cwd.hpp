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

	/**
	 *
	 */
	g_fs_transaction_handler_discovery_set_cwd(char* absolute_path_in, g_contextual<g_syscall_fs_set_working_directory*> data) :
			g_fs_transaction_handler_discovery(absolute_path_in), data(data) {

	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_status perform_afterwork(g_thread* thread) {

		if (status == G_FS_DISCOVERY_SUCCESSFUL) {
			if (!(node->type == G_FS_NODE_TYPE_PIPE || node->type == G_FS_NODE_TYPE_FILE)) {
				g_local<char> node_realpath(new char[G_PATH_MAX]);
				g_filesystem::get_real_path_to_node(node, node_realpath());
				g_string::copy(thread->process->workingDirectory, node_realpath());
				data()->result = G_SET_WORKING_DIRECTORY_SUCCESSFUL;
				g_log_info("%! cwd of process %i is now '%s'", "filesystem", thread->process->main->id, thread->process->workingDirectory);
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
