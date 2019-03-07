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

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/memory.hpp"
#include "shared/system/mutex.hpp"
#include "shared/utils/string.hpp"

static g_fs_node* filesystemRoot;

static g_fs_virt_id filesystemNextId;
static g_mutex filesystemNextIdLock;



void filesystemInitialize() {
	filesystemNextId = 0;

	mutexInitialize(&filesystemNextIdLock);
	filesystemRoot = filesystemCreateNode(G_FS_NODE_TYPE_ROOT, "root");
}

g_fs_node* filesystemCreateNode(g_fs_node_type type, const char* name) {
	g_fs_node* node = (g_fs_node*) heapAllocate(sizeof(g_fs_node));
	node->id = filesystemGetNextId();
	node->type = type;
	node->name = stringDuplicate(name);;
	node->parent = 0;
	node->children = 0;
	node->blocking = false;
	node->upToDate = false;
	return node;
}

g_fs_virt_id filesystemGetNextId() {
	mutexAcquire(&filesystemNextIdLock);
	g_fs_virt_id nextId = filesystemNextId++;
	mutexRelease(&filesystemNextIdLock);
	return nextId;
}

