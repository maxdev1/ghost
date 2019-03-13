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

g_fs_open_status filesystemRamdiskDelegateOpen(g_fs_node* node)
{
	return G_FS_OPEN_SUCCESSFUL;
}

g_fs_close_status filesystemRamdiskDelegateClose(g_fs_node* node)
{
	return G_FS_CLOSE_SUCCESSFUL;
}

g_fs_open_status filesystemRamdiskDelegateDiscover(g_fs_node* parent, const char* name, g_fs_node** outNode)
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

	g_fs_node_type nodeType = ramdiskEntry->type == G_RAMDISK_ENTRY_TYPE_FILE ? G_FS_NODE_TYPE_FILE : G_FS_NODE_TYPE_FOLDER;
	g_fs_node* newNode = filesystemCreateNode(nodeType, name);
	newNode->physicalId = ramdiskEntry->id;
	filesystemAddChild(parent, newNode);
	*outNode = newNode;
	return G_FS_OPEN_SUCCESSFUL;
}

g_fs_read_status filesystemRamdiskDelegateRead(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outRead)
{
	g_ramdisk_entry* entry = ramdiskFindById(node->physicalId);
	if(!entry)
		return G_FS_READ_ERROR;

	if(offset + length > entry->dataSize)
		length = entry->dataSize - offset;

	memoryCopy(buffer, &entry->data[offset], length);
	*outRead = length;
	return G_FS_READ_SUCCESSFUL;
}

g_fs_write_status filesystemRamdiskDelegateWrite(g_fs_node* node, uint8_t* buffer, uint64_t offset, uint64_t length, int64_t* outWrote)
{
	g_ramdisk_entry* entry = ramdiskFindById(node->physicalId);
	if(!entry)
		return G_FS_WRITE_ERROR;

	// copy data from ramdisk memory into variable memory
	if(entry->dataOnRamdisk)
	{
		uint32_t buflen = entry->dataSize * 1.2;
		uint8_t* new_buffer = (uint8_t*) heapAllocate(sizeof(uint8_t) * buflen);
		memoryCopy(new_buffer, entry->data, entry->dataSize);
		entry->data = new_buffer;
		entry->notOnRdBufferLength = buflen;

	} else if(entry->data == nullptr)
	{
		uint32_t initbuflen = 32;
		entry->data = (uint8_t*) heapAllocate(sizeof(uint8_t) * initbuflen);
		entry->notOnRdBufferLength = initbuflen;
	}

	entry->dataOnRamdisk = false;

	// expand buffer until enough space is available
	uint32_t space;
	while((space = entry->notOnRdBufferLength - offset) < length)
	{
		uint32_t buflen = entry->notOnRdBufferLength * 1.2;
		uint8_t* new_buffer = new uint8_t[buflen];
		memoryCopy(new_buffer, entry->data, entry->dataSize);
		heapFree(entry->data);
		entry->data = new_buffer;
		entry->notOnRdBufferLength = buflen;
	}

	// copy data
	memoryCopy(&entry->data[offset], buffer, length);
	entry->dataSize = offset + length;
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
;

g_fs_open_status filesystemRamdiskDelegateTruncate(g_fs_node* file)
{
	g_ramdisk_entry* entry = ramdiskFindById(file->physicalId);
	if(!entry)
		return G_FS_OPEN_ERROR;

	if(!entry->dataOnRamdisk)
	{
		entry->dataSize = 0;
		entry->notOnRdBufferLength = 0;
		heapFree(entry->data);
		entry->data = 0;
	}
	return G_FS_OPEN_SUCCESSFUL;
}
