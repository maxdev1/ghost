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

#include "kernel/filesystem/filesystem_ramdiskdelegate.hpp"
#include "kernel/filesystem/ramdisk.hpp"

#include "kernel/memory/memory.hpp"
#include "kernel/kernel.hpp"
#include "shared/system/mutex.hpp"
#include "shared/utils/string.hpp"

g_fs_node* filesystemRamdiskDelegateDiscoverChild(g_fs_node* parent, const char* name)
{
	g_ramdisk_entry* parentEntry;

	if(parent->type == G_FS_NODE_TYPE_MOUNTPOINT)
		parentEntry = ramdiskGetRoot();
	else
		parentEntry = ramdiskFindById(parent->physicalId);

	if(!parentEntry)
		return 0;

	g_ramdisk_entry* ramdiskEntry = ramdiskFindChild(parentEntry, name);
	if(!ramdiskEntry)
		return 0;

	g_fs_node_type nodeType = ramdiskEntry->type == G_RAMDISK_ENTRY_TYPE_FILE ? G_FS_NODE_TYPE_FILE : G_FS_NODE_TYPE_FOLDER;
	g_fs_node* newNode = filesystemCreateNode(nodeType, name);
	newNode->physicalId = ramdiskEntry->id;
	filesystemAddChild(parent, newNode);
	return newNode;
}
