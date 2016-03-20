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

#ifndef GHOST_FILESYSTEM_FILEDESCRIPTORS
#define GHOST_FILESYSTEM_FILEDESCRIPTORS

#include "ghost/fs.h"
#include "filesystem/pipes.hpp"
#include <utils/hash_map.hpp>
#include <tasking/process.hpp>

/**
 *
 */
struct g_file_descriptor_content {
	g_fd id;
	int64_t offset;
	g_fs_virt_id node_id;
	int32_t open_flags;

	void clone_into(g_file_descriptor_content* other) {
		other->offset = offset;
		other->node_id = node_id;
		other->open_flags = open_flags;
	}
};

/**
 *
 */
struct g_file_descriptor_table {
	g_fd nextFileDescriptor = 3; // skips stdin/stdout/stderr
	g_hash_map<g_fd, g_file_descriptor_content*> descriptors;
};

/**
 *
 */
class g_file_descriptors {
private:
	static g_file_descriptor_content* create_descriptor(g_file_descriptor_table* table, g_fd override_fd = -1);
public:
	static void initialize();

	static g_fd map(g_pid pid, g_fs_virt_id node_id, g_fd fd, int32_t open_flags);
	static void unmap(g_pid pid, g_fd fd);
	static void unmap_all(g_pid pid);

	static g_file_descriptor_content* get(g_pid pid, g_fd fd);
	static g_file_descriptor_table* get_process_table(g_pid pid);
};

#endif
