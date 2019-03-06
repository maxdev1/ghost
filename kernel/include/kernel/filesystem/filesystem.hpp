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

#ifndef __KERNEL_FILESYSTEM__
#define __KERNEL_FILESYSTEM__

#include "ghost/fs.h"

struct g_fs_node;

struct g_fs_node_entry {
	g_fs_node* node;
	g_fs_node_entry* next;
};

struct g_fs_node {
	g_fs_virt_id id;
	g_fs_phys_id physicalId;
	g_fs_node_type type;

	char* name;
	g_fs_node* parent;
	g_fs_node_entry* children;

	bool blocking;
	bool upToDate;
};


void filesystemInitialize();

g_fs_virt_id filesystemGetNextId();

g_fs_node* filesystemCreateNode(g_fs_node_type type, const char* name);

#endif
