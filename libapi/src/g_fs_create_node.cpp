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

#include "ghost/user.h"

/**
 *
 */
g_fs_create_node_status g_fs_create_node(uint32_t parent, char* name, g_fs_node_type type, uint64_t fs_id, uint32_t* out_created_id) {

	g_syscall_fs_create_node data;
	data.parent_id = parent;
	data.name = (char*) name;
	data.type = type;
	data.phys_fs_id = fs_id;
	g_syscall(G_SYSCALL_FS_CREATE_NODE, (uint32_t) &data);
	*out_created_id = data.created_id;
	return data.result;
}
