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
#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/filesystem/filesystem_ramdiskdelegate.hpp"

#include "kernel/memory/memory.hpp"
#include "kernel/kernel.hpp"
#include "shared/system/mutex.hpp"
#include "shared/utils/string.hpp"

static g_fs_node* filesystemRoot;
static g_fs_node* mountFolder;

static g_fs_virt_id filesystemNextId;
static g_mutex filesystemNextIdLock;

void filesystemInitialize()
{
	mutexInitialize(&filesystemNextIdLock);
	filesystemNextId = 0;

	filesystemProcessInitialize();
	filesystemCreateRoot();
}

void filesystemCreateRoot()
{
	g_fs_delegate* ramdiskDelegate = filesystemCreateDelegate();
	ramdiskDelegate->discoverChild = filesystemRamdiskDelegateDiscoverChild;

	filesystemRoot = filesystemCreateNode(G_FS_NODE_TYPE_ROOT, "root");
	filesystemRoot->delegate = ramdiskDelegate;

	mountFolder = filesystemCreateNode(G_FS_NODE_TYPE_FOLDER, "mount");
	filesystemAddChild(filesystemRoot, mountFolder);

	g_fs_node* ramdiskMountpoint = filesystemCreateNode(G_FS_NODE_TYPE_MOUNTPOINT, "ramdisk");
	ramdiskMountpoint->physicalId = 0;
	ramdiskMountpoint->delegate = ramdiskDelegate;
	filesystemAddChild(mountFolder, ramdiskMountpoint);
}

g_fs_node* filesystemCreateNode(g_fs_node_type type, const char* name)
{
	g_fs_node* node = (g_fs_node*) heapAllocate(sizeof(g_fs_node));
	node->id = filesystemGetNextId();
	node->type = type;
	node->name = stringDuplicate(name);
	node->parent = 0;
	node->children = 0;
	node->delegate = 0;
	node->blocking = false;
	node->upToDate = false;
	return node;
}

void filesystemAddChild(g_fs_node* parent, g_fs_node* child)
{
	g_fs_delegate* delegate = filesystemFindDelegate(parent);
	mutexAcquire(&delegate->lock);

	child->parent = parent;

	g_fs_node_entry* entry = (g_fs_node_entry*) heapAllocate(sizeof(g_fs_node_entry));
	entry->node = child;
	entry->next = child->children;
	child->children = entry;

	mutexRelease(&delegate->lock);
}

g_fs_virt_id filesystemGetNextId()
{
	mutexAcquire(&filesystemNextIdLock);
	g_fs_virt_id nextId = filesystemNextId++;
	mutexRelease(&filesystemNextIdLock);
	return nextId;
}

g_fs_node* filesystemFindChild(g_fs_node* parent, const char* name)
{
	if(stringEquals(name, ".."))
	{
		return parent->parent;
	}

	if(stringEquals(name, "."))
	{
		return parent;
	}

	g_fs_node_entry* child = parent->children;
	g_fs_node* lastKnown = parent;
	while(child)
	{
		if(stringEquals(name, child->node->name))
		{
			return child->node;
		}
		lastKnown = child->node;
		child = child->next;
	}

	g_fs_delegate* delegate = filesystemFindDelegate(lastKnown);
	if(!delegate->discoverChild)
		return 0;
	return delegate->discoverChild(lastKnown, name);
}

g_fs_node* filesystemFind(g_fs_node* parent, const char* path)
{
	char* nameBuf = (char*) heapAllocate(sizeof(char) * (G_FILENAME_MAX + 1));

	const char* pathPos = path;
	g_fs_node* node = parent;

	while(pathPos)
	{
		while(*pathPos == '/')
			++pathPos;

		const char* nameEnd = pathPos;
		while(*nameEnd && *nameEnd != '/')
			++nameEnd;

		int nameLen = nameEnd - pathPos;
		if(nameLen == 0)
			break;

		if(nameLen > G_FILENAME_MAX)
		{
			logInfo("%! tried to resolve path with filename (%i) longer than max (%i): %s", "filesystem", nameLen, G_FILENAME_MAX, path);
			heapFree(nameBuf);
			return 0;
		}
		memoryCopy(nameBuf, pathPos, nameLen);
		nameBuf[nameLen] = 0;

		node = filesystemFindChild(node, nameBuf);
		if(!node)
			break;

		pathPos = nameEnd;
	}

	heapFree(nameBuf);
	return node;
}

g_fs_delegate* filesystemCreateDelegate()
{
	g_fs_delegate* delegate = (g_fs_delegate*) heapAllocate(sizeof(g_fs_delegate));
	mutexInitialize(&delegate->lock);
	return delegate;
}

g_fs_delegate* filesystemFindDelegate(g_fs_node* node)
{
	if(!node->delegate && node->parent)
		return filesystemFindDelegate(node->parent);

	g_fs_delegate* delegate = node->delegate;
	if(delegate == 0)
		kernelPanic("%! failed to find delegate for node %i", "filesystem", node->id);
	return delegate;
}
