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

g_fs_open_status filesystemRamdiskDelegateOpen(g_fs_node* node, g_file_flag_mode flags)
{
	return G_FS_OPEN_SUCCESSFUL;
}

g_fs_close_status filesystemRamdiskDelegateClose(g_fs_node* node, g_file_flag_mode openFlags)
{
	return G_FS_CLOSE_SUCCESSFUL;
}

g_fs_open_status filesystemRamdiskDelegateLookup(g_fs_node* parent, const char* name, g_fs_node** outNode)
{
	g_ramdisk_entry* parentEntry;

	if(parent->type == G_FS_NODE_TYPE_MOUNTPOINT)
		parentEntry = ramdiskGetRoot();
	else
		parentEntry = ramdiskFindById(parent->physicalId);

	if(!parentEntry)
	{
		*outNode = 0;
		return G_FS_OPEN_ERROR;
	}

	g_ramdisk_entry* ramdiskEntry = ramdiskFindChild(parentEntry, name);
	if(!ramdiskEntry)
	{
		*outNode = 0;
		return G_FS_OPEN_NOT_FOUND;
	}

	g_fs_node_type nodeType = ramdiskEntry->type == G_RAMDISK_ENTRY_TYPE_FILE
		                          ? G_FS_NODE_TYPE_FILE
		                          : G_FS_NODE_TYPE_FOLDER;
	g_fs_node* newNode = filesystemCreateNode(nodeType, name);
	newNode->physicalId = ramdiskEntry->id;
	filesystemAddChild(parent, newNode);
	*outNode = newNode;
	return G_FS_OPEN_SUCCESSFUL;
}

g_fs_read_status filesystemRamdiskDelegateRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length,
                                               int64_t* outRead)
{
	*outRead = -1;
	g_ramdisk_entry* entry = ramdiskFindById(node->physicalId);
	if(!entry)
		return G_FS_READ_ERROR;

	if(offset >= entry->dataSize || length == 0)
	{
		*outRead = 0;
		return G_FS_READ_SUCCESSFUL;
	}
	uint64_t available = entry->dataSize - offset;
	if(length > available)
		length = available;
	if(length > INT32_MAX)
		length = INT32_MAX;

	memoryCopy(buffer, &entry->data[offset], length);
	*outRead = length;
	return G_FS_READ_SUCCESSFUL;
}

g_fs_write_status filesystemRamdiskDelegateWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length,
                                                 int64_t* outWrote)
{
	*outWrote = -1;
	g_ramdisk_entry* entry = ramdiskFindById(node->physicalId);
	if(!entry)
		return G_FS_WRITE_ERROR;
	if(length == 0)
	{
		*outWrote = 0;
		return G_FS_WRITE_SUCCESSFUL;
	}
	if(offset > INT32_MAX || length > INT32_MAX - offset || entry->dataSize > INT32_MAX)
		return G_FS_WRITE_ERROR;

	uint32_t end = offset + length;
	if(entry->dataOnRamdisk || end > entry->notOnRdBufferLength)
	{
		uint32_t capacity = entry->dataOnRamdisk ? entry->dataSize : entry->notOnRdBufferLength;
		if(capacity < 32)
			capacity = 32;
		if(capacity < end)
		{
			uint64_t grown = uint64_t(capacity) * 2;
			capacity = grown > INT32_MAX ? INT32_MAX : grown;
			if(capacity < end)
				capacity = end;
		}
		auto* data = static_cast<uint8_t*>(heapAllocate(capacity));
		if(!data)
			return G_FS_WRITE_ERROR;
		if(entry->dataSize)
			memoryCopy(data, entry->data, entry->dataSize);
		if(!entry->dataOnRamdisk && entry->data)
			heapFree(entry->data);
		entry->data = data;
		entry->notOnRdBufferLength = capacity;
		entry->dataOnRamdisk = false;
	}

	if(offset > entry->dataSize)
		memorySetBytes(entry->data + entry->dataSize, 0, offset - entry->dataSize);
	memoryCopy(entry->data + offset, buffer, length);
	if(end > entry->dataSize)
		entry->dataSize = end;
	*outWrote = length;
	return G_FS_WRITE_SUCCESSFUL;
}

g_fs_length_status filesystemRamdiskDelegateGetLength(g_fs_node* node, uint64_t* outLength)
{
	g_ramdisk_entry* entry = ramdiskFindById(node->physicalId);
	if(!entry)
		return G_FS_LENGTH_ERROR;

	*outLength = entry->dataSize;
	return G_FS_LENGTH_SUCCESSFUL;
}

g_fs_open_status filesystemRamdiskDelegateCreate(g_fs_node* parent, const char* name, g_fs_node** outFile)
{
	g_ramdisk_entry* entry = ramdiskFindById(parent->physicalId);
	if(!entry)
		return G_FS_OPEN_ERROR;

	g_ramdisk_entry* newFile = ramdiskCreateFile(entry, name);

	g_fs_node* newNode = filesystemCreateNode(G_FS_NODE_TYPE_FILE, name);
	newNode->physicalId = newFile->id;
	filesystemAddChild(parent, newNode);
	*outFile = newNode;

	return G_FS_OPEN_SUCCESSFUL;
}

g_fs_open_status filesystemRamdiskDelegateTruncate(g_fs_node* file)
{
	g_ramdisk_entry* entry = ramdiskFindById(file->physicalId);
	if(!entry)
		return G_FS_OPEN_ERROR;

	if(!entry->dataOnRamdisk && entry->data)
		heapFree(entry->data);
	entry->dataOnRamdisk = false;
	entry->dataSize = 0;
	entry->notOnRdBufferLength = 0;
	entry->data = nullptr;
	return G_FS_OPEN_SUCCESSFUL;
}

g_fs_directory_refresh_status filesystemRamdiskDelegateRefreshDir(g_fs_node* dir)
{
	g_ramdisk_entry* dirEntry = ramdiskFindById(dir->physicalId);
	if(!dirEntry)
		return G_FS_DIRECTORY_REFRESH_ERROR;

	auto childCount = ramdiskGetChildCount(dirEntry->id);

	for(uint32_t i = 0; i < childCount; i++)
	{
		auto childEntry = ramdiskGetChildAt(dirEntry->id, i);

		if(!filesystemFindExistingChild(dir, childEntry->name))
		{
			g_fs_node_type nodeType = childEntry->type == G_RAMDISK_ENTRY_TYPE_FILE
				                          ? G_FS_NODE_TYPE_FILE
				                          : G_FS_NODE_TYPE_FOLDER;
			g_fs_node* newNode = filesystemCreateNode(nodeType, childEntry->name);
			newNode->physicalId = childEntry->id;
			filesystemAddChild(dir, newNode);
		}
	}

	return G_FS_DIRECTORY_REFRESH_SUCCESSFUL;
}
