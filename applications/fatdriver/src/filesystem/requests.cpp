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

#include <cstring>
#include <new>
#include "mutation.hpp"
#include "volume/mount.hpp"

static g_fs_delegate_status fatPublishNode(g_fat_mount* mount, g_fs_virt_id parent, const g_fat_directory_entry& entry,
		g_fs_virt_id* id)
{
	return g_fs_delegate_node_create(mount->storage->header.transaction, parent, entry.name,
		entry.node.directory ? G_FS_NODE_TYPE_FOLDER : G_FS_NODE_TYPE_FILE, entry.node.id,
		id);
}

static void fatHandleLookup(g_fat_mount* mount)
{
	auto& lookup = mount->storage->data.lookup;
	lookup.result_status = G_FS_LOOKUP_ERROR;
	lookup.result_node_id = G_FS_VIRT_ID_NONE;

	g_fat_node parent;
	g_fat_directory directory;
	if(!fatNodeRead(mount, lookup.parent_phys_fs_id, &parent))
		return;
	if(!fatDirectoryOpen(mount, parent, &directory))
		return;

	for(;;)
	{
		g_fat_directory_entry entry;
		auto result = fatDirectoryNext(mount, &directory, &entry);
		if(result == G_FAT_END)
		{
			lookup.result_status = G_FS_LOOKUP_NOT_FOUND;
			return;
		}

		if(result != G_FAT_SUCCESS)
			return;
		if(!fatNameEquals(lookup.name, entry.name) && !fatNameEquals(lookup.name, entry.alias))
			continue;

		if(fatPublishNode(mount, lookup.parent_virt_fs_id, entry, &lookup.result_node_id) == G_FS_DELEGATE_SUCCESSFUL)
			lookup.result_status = G_FS_LOOKUP_SUCCESSFUL;
		return;
	}
}

static void fatHandleDirectory(g_fat_mount* mount)
{
	auto& refresh = mount->storage->data.directory_refresh;
	refresh.result_status = G_FS_DIRECTORY_REFRESH_ERROR;

	g_fat_node parent;
	g_fat_directory directory;
	if(!fatNodeRead(mount, refresh.parent_phys_fs_id, &parent))
		return;
	if(!fatDirectoryOpen(mount, parent, &directory))
		return;

	for(;;)
	{
		g_fat_directory_entry entry;
		auto result = fatDirectoryNext(mount, &directory, &entry);
		if(result == G_FAT_END)
		{
			refresh.result_status = G_FS_DIRECTORY_REFRESH_SUCCESSFUL;
			return;
		}

		if(result != G_FAT_SUCCESS)
			return;

		g_fs_virt_id id;
		if(fatPublishNode(mount, refresh.parent_virt_fs_id, entry, &id) != G_FS_DELEGATE_SUCCESSFUL)
			return;
	}
}

static void fatHandleRead(g_fat_mount* mount)
{
	auto& read = mount->storage->data.read;
	read.result_status = G_FS_READ_ERROR;
	read.result_read = -1;

	g_fat_node node;
	if(!fatNodeRead(mount, read.phys_fs_id, &node))
		return;

	size_t count;
	if(!fatFileRead(mount, node, read.offset, read.mapped_buffer, read.length, &count))
		return;

	read.result_read = count;
	read.result_status = G_FS_READ_SUCCESSFUL;
}

static void fatHandleLength(g_fat_mount* mount)
{
	auto& length = mount->storage->data.get_length;
	length.result_status = G_FS_LENGTH_ERROR;
	length.result_length = 0;

	g_fat_node node;
	if(fatNodeRead(mount, length.phys_fs_id, &node))
	{
		length.result_length = node.size;
		length.result_status = G_FS_LENGTH_SUCCESSFUL;
	}
}

static void fatHandleOpen(g_fat_mount* mount)
{
	auto& open = mount->storage->data.open;
	open.result_status = G_FS_OPEN_ERROR;
	g_fat_node node;
	if(!fatNodeRead(mount, open.phys_fs_id, &node) || node.directory)
		return;

	auto inode = fatInodeFind(mount, open.phys_fs_id);
	if(!inode || inode->opens == UINT32_MAX)
		return;
	if(open.flags & (G_FILE_FLAG_MODE_WRITE | G_FILE_FLAG_MODE_APPEND))
	{
		if(fatMutationAllowed(mount, inode) != G_FS_MUTATION_SUCCESSFUL)
			return;
	}

	++inode->opens;
	open.result_status = G_FS_OPEN_SUCCESSFUL;
}

static void fatHandleWrite(g_fat_mount* mount)
{
	auto& write = mount->storage->data.write;
	write.result_status = G_FS_WRITE_ERROR;
	write.result_write = -1;
	size_t count;
	write.result_status =
			fatFileWrite(mount, write.phys_fs_id, write.offset, write.mapped_buffer, write.length, &count);
	if(write.result_status == G_FS_WRITE_SUCCESSFUL)
		write.result_write = count;
}

static void fatHandleCreate(g_fat_mount* mount)
{
	auto& create = mount->storage->data.create;
	create.result_status = G_FS_MUTATION_ERROR;
	create.result_node_id = G_FS_VIRT_ID_NONE;
	g_fat_directory_entry entry;
	create.result_status = fatDirectoryCreate(mount, create.parent_phys_fs_id, create.name,
		create.type == G_FS_NODE_TYPE_FOLDER, &entry);
	if(create.result_status != G_FS_MUTATION_SUCCESSFUL)
		return;

	auto status = fatPublishNode(mount, create.parent_virt_fs_id, entry, &create.result_node_id);
	if(status != G_FS_DELEGATE_SUCCESSFUL)
		create.result_status = G_FS_MUTATION_ERROR;
}

static void fatHandleResize(g_fat_mount* mount)
{
	auto& resize = mount->storage->data.resize;
	resize.result_status = fatFileResize(mount, resize.phys_fs_id, resize.length);
}

static void fatHandleRemove(g_fat_mount* mount)
{
	auto& remove = mount->storage->data.remove;
	remove.result_status = fatNodeRemove(mount, remove.phys_fs_id);
}

static void fatHandleRename(g_fat_mount* mount)
{
	auto& rename = mount->storage->data.rename;
	rename.result_status = fatNodeRename(mount, rename.phys_fs_id, rename.parent_phys_fs_id,
		rename.name, rename.replaced_phys_fs_id);
}

static void fatHandleFlush(g_fat_mount* mount)
{
	auto& flush = mount->storage->data.flush;
	if(fatMountFlush(mount))
		flush.result_status = G_FS_MUTATION_SUCCESSFUL;
	else
		flush.result_status = G_FS_MUTATION_ERROR;
}

static void fatHandleClose(g_fat_mount* mount)
{
	auto& close = mount->storage->data.close;
	auto status = fatInodeClose(mount, close.phys_fs_id);
	if(status == G_FS_MUTATION_SUCCESSFUL)
		close.result_status = G_FS_CLOSE_SUCCESSFUL;
	else
		close.result_status = G_FS_CLOSE_ERROR;
}

bool fatMountHandleRequest(g_fat_mount* mount, const g_message_header* message, const void* content)
{
	if(message->sender != G_FS_DELEGATE_MESSAGE_SENDER || message->length != sizeof(g_fs_delegate_request))
		return true;

	g_fs_delegate_request request;
	memcpy(&request, content, sizeof(request));
	if(request.version != G_FS_DELEGATE_VERSION || request.storage_size != sizeof(*mount->storage) ||
	   request.mountpoint_id != mount->id || request.transaction == G_FS_TRANSACTION_NONE ||
	   memcmp(&request, &mount->storage->header, sizeof(request)))
		return true;

	try
	{
		switch(request.type)
		{
			case G_FS_DELEGATE_REQUEST_LOOKUP:
				fatHandleLookup(mount);
				break;
			case G_FS_DELEGATE_REQUEST_READ_DIRECTORY:
				fatHandleDirectory(mount);
				break;
			case G_FS_DELEGATE_REQUEST_READ:
				fatHandleRead(mount);
				break;
			case G_FS_DELEGATE_REQUEST_GET_LENGTH:
				fatHandleLength(mount);
				break;
			case G_FS_DELEGATE_REQUEST_OPEN:
				fatHandleOpen(mount);
				break;
			case G_FS_DELEGATE_REQUEST_WRITE:
				fatHandleWrite(mount);
				break;
			case G_FS_DELEGATE_REQUEST_CREATE:
				fatHandleCreate(mount);
				break;
			case G_FS_DELEGATE_REQUEST_RESIZE:
				fatHandleResize(mount);
				break;
			case G_FS_DELEGATE_REQUEST_REMOVE:
				fatHandleRemove(mount);
				break;
			case G_FS_DELEGATE_REQUEST_RENAME:
				fatHandleRename(mount);
				break;
			case G_FS_DELEGATE_REQUEST_FLUSH:
				fatHandleFlush(mount);
				break;
			case G_FS_DELEGATE_REQUEST_CLOSE:
				fatHandleClose(mount);
				break;
			default:
				return false;
		}
	}
	catch(const std::bad_alloc&)
	{
		mount->failed = true;
		switch(request.type)
		{
			case G_FS_DELEGATE_REQUEST_CREATE:
				mount->storage->data.create.result_status = G_FS_MUTATION_ERROR;
				break;
			case G_FS_DELEGATE_REQUEST_RESIZE:
				mount->storage->data.resize.result_status = G_FS_MUTATION_ERROR;
				break;
			case G_FS_DELEGATE_REQUEST_REMOVE:
				mount->storage->data.remove.result_status = G_FS_MUTATION_ERROR;
				break;
			case G_FS_DELEGATE_REQUEST_RENAME:
				mount->storage->data.rename.result_status = G_FS_MUTATION_ERROR;
				break;
			case G_FS_DELEGATE_REQUEST_WRITE:
				mount->storage->data.write.result_status = G_FS_WRITE_ERROR;
				break;
			case G_FS_DELEGATE_REQUEST_CLOSE:
				mount->storage->data.close.result_status = G_FS_CLOSE_ERROR;
				break;
			default:
				return false;
		}
	}

	bool opened = request.type == G_FS_DELEGATE_REQUEST_OPEN &&
		mount->storage->data.open.result_status == G_FS_OPEN_SUCCESSFUL;
	auto openedId = opened ? mount->storage->data.open.phys_fs_id : G_FAT_ROOT;
	auto status = g_fs_delegate_transaction_complete(request.transaction);
	if(status == G_FS_DELEGATE_STALE_TRANSACTION)
	{
		if(opened)
			fatInodeClose(mount, openedId);
	}

	return status == G_FS_DELEGATE_SUCCESSFUL || status == G_FS_DELEGATE_STALE_TRANSACTION;
}
