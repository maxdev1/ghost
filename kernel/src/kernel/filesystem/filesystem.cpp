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
#include "kernel/tasking/tasking.hpp"

#include "kernel/memory/memory.hpp"
#include "kernel/kernel.hpp"
#include "shared/system/mutex.hpp"
#include "shared/utils/string.hpp"

static g_fs_node* filesystemRoot;
static g_fs_node* mountFolder;

static g_fs_virt_id filesystemNextNodeId;
static g_mutex filesystemNextNodeIdLock;

static g_hashmap<g_fs_virt_id, g_fs_node*>* filesystemNodes;

void filesystemInitialize()
{
	mutexInitialize(&filesystemNextNodeIdLock);
	filesystemNextNodeId = 0;

	filesystemNodes = hashmapCreateNumeric<g_fs_virt_id, g_fs_node*>(1024);

	filesystemProcessInitialize();
	filesystemCreateRoot();
}

void filesystemCreateRoot()
{
	g_fs_delegate* ramdiskDelegate = filesystemCreateDelegate();
	ramdiskDelegate->discover = filesystemRamdiskDelegateDiscover;
	ramdiskDelegate->read = filesystemRamdiskDelegateRead;
	ramdiskDelegate->write = filesystemRamdiskDelegateWrite;
	ramdiskDelegate->truncate = filesystemRamdiskDelegateTruncate;
	ramdiskDelegate->create = filesystemRamdiskDelegateCreate;
	ramdiskDelegate->getLength = filesystemRamdiskDelegateGetLength;

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
	node->id = filesystemGetNextNodeId();
	node->type = type;
	node->name = stringDuplicate(name);
	node->parent = 0;
	node->children = 0;
	node->delegate = 0;
	node->blocking = false;
	node->upToDate = false;

	hashmapPut<g_fs_virt_id, g_fs_node*>(filesystemNodes, node->id, node);
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

g_fs_virt_id filesystemGetNextNodeId()
{
	mutexAcquire(&filesystemNextNodeIdLock);
	g_fs_virt_id nextId = filesystemNextNodeId++;
	mutexRelease(&filesystemNextNodeIdLock);
	return nextId;
}

g_fs_open_status filesystemFindChild(g_fs_node* parent, const char* name, g_fs_node** outChild)
{
	if(stringEquals(name, ".."))
	{
		*outChild = parent->parent;
		return G_FS_OPEN_SUCCESSFUL;
	}

	if(stringEquals(name, "."))
	{
		*outChild = parent;
		return G_FS_OPEN_SUCCESSFUL;
	}

	g_fs_node_entry* child = parent->children;
	g_fs_node* lastKnown = parent;
	while(child)
	{
		if(stringEquals(name, child->node->name))
		{
			*outChild = child->node;
			return G_FS_OPEN_SUCCESSFUL;
		}
		lastKnown = child->node;
		child = child->next;
	}

	g_fs_delegate* delegate = filesystemFindDelegate(lastKnown);
	if(!delegate->discover)
	{
		*outChild = 0;
		return G_FS_OPEN_ERROR;
	}
	return delegate->discover(lastKnown, name, outChild);
}

g_fs_open_status filesystemFind(g_fs_node* parent, const char* path, g_fs_node** outChild, bool* outFoundAllButLast, g_fs_node** outLastFoundParent, const char** outFileNameStart)
{
	if(parent == 0)
	{
		parent = filesystemRoot;
	}
	char* nameBuf = (char*) heapAllocate(sizeof(char) * (G_FILENAME_MAX + 1));

	const char* nameStart = path;
	const char* nameEnd = nameStart;
	g_fs_node* node = parent;
	g_fs_node* lastFoundParent = node;
	g_fs_open_status status = G_FS_OPEN_SUCCESSFUL;

	while(nameStart)
	{
		// Find where next name starts and ends
		while(*nameStart == '/')
			++nameStart;

		nameEnd = nameStart;
		while(*nameEnd && *nameEnd != '/')
			++nameEnd;

		// Nothing left to resolve, we found the node
		int nameLen = nameEnd - nameStart;
		if(nameLen == 0)
			break;

		if(nameLen > G_FILENAME_MAX)
		{
			logInfo("%! tried to resolve path with filename (%i) longer than max (%i): %s", "filesystem", nameLen, G_FILENAME_MAX, path);
			heapFree(nameBuf);
			return 0;
		}

		// Copy name into temporary buffer
		memoryCopy(nameBuf, nameStart, nameLen);
		nameBuf[nameLen] = 0;

		// Find child with this name
		lastFoundParent = node;
		status = filesystemFindChild(node, nameBuf, &node);
		if(status != G_FS_OPEN_SUCCESSFUL)
			break;

		nameStart = nameEnd;
	}

	// Provide output values if requested
	if(outFoundAllButLast)
		*outFoundAllButLast = ((nameEnd - nameStart) > 0);
	if(outLastFoundParent)
		*outLastFoundParent = lastFoundParent;
	if(outFileNameStart)
		*outFileNameStart = nameStart;

	heapFree(nameBuf);
	*outChild = node;
	return status;
}

g_fs_node* filesystemGetNode(g_fs_virt_id id)
{
	return hashmapGet<g_fs_virt_id, g_fs_node*>(filesystemNodes, id, 0);
}

g_fs_node* filesystemGetRoot()
{
	return filesystemRoot;
}

g_fs_delegate* filesystemCreateDelegate()
{
	g_fs_delegate* delegate = (g_fs_delegate*) heapAllocate(sizeof(g_fs_delegate));
	mutexInitialize(&delegate->lock);
	return delegate;
}

g_fs_delegate* filesystemFindDelegate(g_fs_node* node)
{
	if(!node)
		kernelPanic("%! tried to find delegate for null node", "filesystem");

	if(!node->delegate && node->parent)
		return filesystemFindDelegate(node->parent);

	g_fs_delegate* delegate = node->delegate;
	if(delegate == 0)
		kernelPanic("%! failed to find delegate for node %i", "filesystem", node->id);
	return delegate;
}

g_fs_read_status filesystemRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead)
{
	g_fs_delegate* delegate = filesystemFindDelegate(node);
	if(!delegate->read)
		return G_FS_READ_ERROR;

	return delegate->read(node, buffer, offset, length, outRead);
}

g_fs_length_status filesystemGetLength(g_fs_node* node, uint64_t* outLength)
{
	g_fs_delegate* delegate = filesystemFindDelegate(node);
	if(!delegate->getLength)
		return G_FS_LENGTH_ERROR;

	return delegate->getLength(node, outLength);
}

g_fs_write_status filesystemWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote)
{
	g_fs_delegate* delegate = filesystemFindDelegate(node);
	if(!delegate->write)
		return G_FS_WRITE_ERROR;

	return delegate->write(node, buffer, offset, length, outWrote);
}

g_fs_open_status filesystemCreateFile(g_fs_node* parent, const char* name, g_fs_node** outFile)
{
	g_fs_delegate* delegate = filesystemFindDelegate(parent);
	if(!delegate->create)
		return G_FS_OPEN_ERROR;

	return delegate->create(parent, name, outFile);
}

g_fs_open_status filesystemTruncate(g_fs_node* file)
{
	g_fs_delegate* delegate = filesystemFindDelegate(file);
	if(!delegate->truncate)
		return G_FS_OPEN_ERROR;

	return delegate->truncate(file);
}
