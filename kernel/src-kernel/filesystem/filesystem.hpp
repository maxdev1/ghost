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

#ifndef GHOST_FILESYSTEM_FILESYSTEM
#define GHOST_FILESYSTEM_FILESYSTEM

#include "filesystem/fs_delegate.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_transaction_handler.hpp"
#include "filesystem/fs_transaction_handler_read.hpp"
#include "filesystem/fs_transaction_handler_read_directory.hpp"
#include "filesystem/fs_transaction_handler_write.hpp"
#include "filesystem/fs_transaction_handler_discovery.hpp"
#include "filesystem/fs_transaction_handler_get_length.hpp"

#include "ghost/stdint.h"
#include <tasking/tasking.hpp>
#include "debug/debug_interface_kernel.hpp"

/**
 *
 */
class g_filesystem {
public:

	/**
	 * Initializes the filesystem, allocating & preparing all necessary data structures
	 * that are used.
	 */
	static void initialize();

	/**
	 *
	 */
	static g_fs_node* get_root();

	/**
	 *
	 */
	static g_fs_node* get_node_by_id(g_fs_virt_id id);

	/**
	 * Creates a new filesystem node, assigning a unique node id and
	 * adding it to the global map of nodes.
	 *
	 * @return the created node
	 */
	static g_fs_node* create_node();

	/**
	 * Tries to resolve the node with the given absolute path. The buffer behind
	 * "name_current" is filled with the name of the last found node (must be of at
	 * least G_PATH_MAX bytes size).
	 *
	 * - If the node is found, parent and child are set
	 * - If the node is NOT found, parent is set, the child is set to 0
	 */
	static void find_existing(char* absolute_path, g_fs_node** out_parent, g_fs_node** out_child, char* name_current, bool follow_symlinks = true);

	/**
	 * Resolves the real path to the given node and writes it to the out buffer.
	 *
	 * @param node
	 * 		the node to retrieve the path for
	 *
	 * @param out
	 * 		the output buffer with a size of at least {G_PATH_MAX} bytes
	 */
	static void get_real_path_to_node(g_fs_node* node, char* out);

	/**
	 * Tries to create a mountpoint with the given name and registers the given thread
	 * as the delegate.
	 *
	 * @param thread
	 * 		the thread that is used as the delegate
	 *
	 * @param name
	 * 		name for the mountpoint
	 *
	 * @param phys_mountpoint_id
	 * 		the physical mountpoint id to send the delegate when discovering children
	 *
	 * @param out_mountpoint_id
	 * 		is filled with the generated mountpoint id on success
	 *
	 * @param out_transaction_storage
	 * 		is filled with the address of the transaction storage on success
	 *
	 * @return one of the {g_fs_register_as_delegate_status} codes
	 */
	static g_fs_register_as_delegate_status create_delegate(g_thread* thread, char* name, g_fs_phys_id phys_mountpoint_id, g_fs_virt_id* out_mountpoint_id,
			g_address* out_transaction_storage);

	static bool node_for_descriptor(g_pid pid, g_fd fd, g_fs_node** out_node, g_file_descriptor_content** out_fd);

	/**
	 * If the in path is absolute (starts with a '/') it is simply copied to the out buffer.
	 * If the in path is a relative path, it is appended to the working directory (adding
	 * a '/' in between) and written to the out buffer.
	 *
	 * @param relative_base
	 * 		the relative base to append on relative paths
	 *
	 * @param in
	 * 		the absolute or relative path
	 *
	 * @param out
	 * 		the output buffer of at least {G_PATH_MAX} bytes size
	 */
	static void concat_as_absolute_path(char* relative_base, char* in, char* out);

	/**
	 * Opens a file, creating a file descriptor for the given node within
	 * the given process.
	 *
	 * @param pid
	 * 		id of the process
	 *
	 * @param node
	 * 		the node to open
	 *
	 * @param flags
	 * 		open mode flags
	 *
	 * @param-opt fd
	 * 		the file descriptor to use
	 *
	 * @return a file descriptor
	 */
	static g_fd map_file(g_pid pid, g_fs_node* node, int32_t open_flags, g_fd fd = -1);

	/**
	 *
	 */
	static bool unmap_file(g_pid pid, g_fs_node* node, g_file_descriptor_content* fd);

	/**
	 *
	 */
	static g_fd clonefd(g_fd source_fd, g_pid source_pid, g_fd target_fd, g_pid target_pid, g_fs_clonefd_status* out_status);

	/**
	 *
	 */
	static g_fs_pipe_status pipe(g_thread* thread, bool blocking, g_fd* out_write, g_fd* out_read);

	/**
	 *
	 */
	static void process_closed(g_pid pid);

	/**
	 *
	 */
	static void process_forked(g_pid source, g_pid fork);

};

#endif
