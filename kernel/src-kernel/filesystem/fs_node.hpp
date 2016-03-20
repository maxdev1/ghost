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

#ifndef GHOST_FILESYSTEM_FILESYSTEMNODE
#define GHOST_FILESYSTEM_FILESYSTEMNODE

#include "ghost/stdint.h"
#include "ghost/fs.h"
#include "utils/list_entry.hpp"

class g_fs_delegate;

/**
 *
 */
class g_fs_node {
private:
	g_fs_delegate* delegate;

public:
	g_fs_node();

	void set_delegate(g_fs_delegate* delegate);
	g_fs_delegate* get_delegate();

	g_fs_node_type type;
	g_fs_virt_id id;
	g_fs_phys_id phys_fs_id;

	char* name;
	g_fs_node* parent;
	g_list_entry<g_fs_node*>* children;
	void add_child(g_fs_node* child);

	bool is_blocking;

	/**
	 * True if the node is a directory and all child
	 * nodes are up-to-date.
	 */
	bool contents_valid;

	g_fs_node* find_child(char* name);
};

#endif
