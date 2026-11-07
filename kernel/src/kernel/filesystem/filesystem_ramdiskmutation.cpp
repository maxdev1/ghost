/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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
#include "kernel/utils/string.hpp"

void filesystemRamdiskDelegateReclaim(g_fs_node* node)
{
	auto entry = ramdiskFindById(node->physicalId);
	if(!entry || entry->opens || entry->parentid != UINT32_MAX)
		return;

	if(!entry->dataOnRamdisk && entry->data)
		heapFree(entry->data);
	entry->data = nullptr;
	entry->dataSize = 0;
	entry->notOnRdBufferLength = 0;
	entry->dataOnRamdisk = false;
}

g_fs_mutation_status filesystemRamdiskDelegateMakeDirectory(g_fs_node* parent, const char* name, g_fs_node** out)
{
	g_mutex_guard namespaceGuard(&filesystemFindDelegate(parent)->lock);

	auto entry = ramdiskFindById(parent->physicalId);
	if(!entry)
		return G_FS_MUTATION_NOT_FOUND;
	if(ramdiskFindChild(entry, name))
		return G_FS_MUTATION_ALREADY_EXISTS;

	auto created = ramdiskCreateFile(entry, name);
	created->type = G_RAMDISK_ENTRY_TYPE_FOLDER;

	auto node = filesystemCreateNode(G_FS_NODE_TYPE_FOLDER, name);
	node->physicalId = created->id;

	filesystemAddChild(parent, node);

	*out = node;
	return G_FS_MUTATION_SUCCESSFUL;
}

g_fs_mutation_status filesystemRamdiskDelegateMutate(g_fs_node* node, g_fs_delegate_request_type type, uint64_t length)
{
	g_mutex_guard nodeGuard(&node->lock);

	auto entry = ramdiskFindById(node->physicalId);
	if(!entry)
		return G_FS_MUTATION_NOT_FOUND;
	if(type == G_FS_DELEGATE_REQUEST_FLUSH)
		return G_FS_MUTATION_SUCCESSFUL;

	if(type == G_FS_DELEGATE_REQUEST_REMOVE)
	{
		if(entry == ramdiskGetRoot() || (node->type != G_FS_NODE_TYPE_FILE && node->type != G_FS_NODE_TYPE_FOLDER &&
										 node->type != G_FS_NODE_TYPE_SYMLINK))
			return G_FS_MUTATION_BUSY;
		if(node->type == G_FS_NODE_TYPE_FOLDER && ramdiskGetChildCount(entry->id))
			return G_FS_MUTATION_NOT_EMPTY;

		entry->parentid = UINT32_MAX;
		filesystemUnlinkNode(node);
		filesystemRamdiskDelegateReclaim(node);
		return G_FS_MUTATION_SUCCESSFUL;
	}

	if(type != G_FS_DELEGATE_REQUEST_RESIZE)
		return G_FS_MUTATION_NOT_SUPPORTED;
	if(entry->type != G_RAMDISK_ENTRY_TYPE_FILE || length > INT32_MAX)
		return G_FS_MUTATION_INVALID_ARGUMENT;

	if(length > entry->dataSize)
	{
		auto data = static_cast<uint8_t*>(heapTryAllocate(length));
		if(!data)
			return G_FS_MUTATION_NO_SPACE;
		if(entry->dataSize)
			memoryCopy(data, entry->data, entry->dataSize);

		memorySetBytes(data + entry->dataSize, 0, length - entry->dataSize);
		if(!entry->dataOnRamdisk && entry->data)
			heapFree(entry->data);

		entry->data = data;
		entry->notOnRdBufferLength = length;
		entry->dataOnRamdisk = false;
	}

	entry->dataSize = length;
	return G_FS_MUTATION_SUCCESSFUL;
}

g_fs_mutation_status filesystemRamdiskDelegateRename(g_fs_node* node, g_fs_node* parent, const char* name,
													 g_fs_node* replaced)
{
	auto directory = ramdiskFindById(parent->physicalId);

	auto entry = ramdiskFindById(node->physicalId);
	if(!entry || !directory)
		return G_FS_MUTATION_NOT_FOUND;
	if(entry == ramdiskGetRoot())
		return G_FS_MUTATION_BUSY;

	auto existing = ramdiskFindChild(directory, name);
	if(existing && (!replaced || existing->id != replaced->physicalId))
		return G_FS_MUTATION_ALREADY_EXISTS;

	auto copy = static_cast<char*>(heapTryAllocate(stringLength(name) + 1));
	if(!copy)
		return G_FS_MUTATION_ERROR;

	stringCopy(copy, name);

	if(replaced)
	{
		auto result = filesystemRamdiskDelegateMutate(replaced, G_FS_DELEGATE_REQUEST_REMOVE, 0);
		if(result != G_FS_MUTATION_SUCCESSFUL)
		{
			heapFree(copy);
			return result;
		}
	}

	auto previous = entry->name;
	entry->name = copy;
	entry->parentid = directory->id;
	heapFree(previous);

	return G_FS_MUTATION_SUCCESSFUL;
}
