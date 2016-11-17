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

#include "filesystem/fs_delegate_ramdisk.hpp"
#include "filesystem/filesystem.hpp"
#include "utils/string.hpp"
#include "logger/logger.hpp"
#include "kernel.hpp"
#include "ghost/utils/local.hpp"

/**
 *
 */
g_fs_node* g_fs_delegate_ramdisk::create_vfs_node(g_ramdisk_entry* ramdisk_node, g_fs_node* parent) {

	g_fs_node* node = g_filesystem::create_node();
	node->phys_fs_id = ramdisk_node->id;

	if (ramdisk_node->type == G_RAMDISK_ENTRY_TYPE_FILE) {
		node->type = G_FS_NODE_TYPE_FILE;
	} else {
		node->type = G_FS_NODE_TYPE_FOLDER;
	}

	int len = g_string::length(ramdisk_node->name);
	node->name = new char[len + 1];
	g_memory::copy(node->name, ramdisk_node->name, len);
	node->name[len] = 0;

	// add it to the parent
	parent->add_child(node);
	G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(node);

	return node;
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_discovery(g_thread* requester, g_fs_node* parent, char* child, g_fs_transaction_handler_discovery* handler) {

	// the ramdisk handler is doing it's work immediately and doesn't request another process
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	// find on ramdisk
	g_ramdisk_entry* ramdisk_parent;
	if (parent->type == G_FS_NODE_TYPE_MOUNTPOINT) {
		ramdisk_parent = g_kernel::ramdisk->getRoot();
	} else {
		ramdisk_parent = g_kernel::ramdisk->findById(parent->phys_fs_id);
	}

	if (ramdisk_parent) {
		g_ramdisk_entry* ramdisk_node = g_kernel::ramdisk->findChild(ramdisk_parent, child);

		if (ramdisk_node) {
			// create the VFS node
			create_vfs_node(ramdisk_node, parent);
			handler->status = G_FS_DISCOVERY_SUCCESSFUL;
		} else {
			handler->status = G_FS_DISCOVERY_NOT_FOUND;
		}
	} else {
		handler->status = G_FS_DISCOVERY_NOT_FOUND;
	}
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_discovery(g_thread* requester, g_fs_transaction_handler_discovery* handler) {
	// nothing to do here
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_read(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_read* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	g_ramdisk_entry* ramdisk_node = g_kernel::ramdisk->findById(node->phys_fs_id);
	if (ramdisk_node == 0) {
		handler->status = G_FS_READ_INVALID_FD;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		return id;
	}

	// check if node is valid
	if (ramdisk_node->data == nullptr) {
		g_log_warn("%! tried to read from a node %i that has no buffer", "ramdisk", node->phys_fs_id);
		handler->status = G_FS_READ_ERROR;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		return id;
	}

	// read data into buffer
	int64_t copy_amount = ((fd->offset + length) >= ramdisk_node->datalength) ? (ramdisk_node->datalength - fd->offset) : length;
	if (copy_amount > 0) {
		g_memory::copy(buffer(), &ramdisk_node->data[fd->offset], copy_amount);
		fd->offset += copy_amount;
	}
	handler->result = copy_amount;
	handler->status = G_FS_READ_SUCCESSFUL;
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);

	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_read(g_thread* requester, g_fs_read_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_write(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
		g_file_descriptor_content* fd, g_fs_transaction_handler_write* handler) {

	// start/repeat transaction
	g_fs_transaction_id id;
	if (handler->wants_repeat_transaction()) {
		id = handler->get_repeated_transaction();
	} else {
		id = g_fs_transaction_store::next_transaction();
	}

	g_ramdisk_entry* ramdisk_node = g_kernel::ramdisk->findById(node->phys_fs_id);
	if (ramdisk_node == 0) {
		handler->status = G_FS_WRITE_INVALID_FD;
		g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
		return id;
	}

	// copy data from ramdisk memory into variable memory
	if (ramdisk_node->data_on_ramdisk) {
		uint32_t buflen = ramdisk_node->datalength * 1.2;
		uint8_t* new_buffer = new uint8_t[buflen];
		g_memory::copy(new_buffer, ramdisk_node->data, ramdisk_node->datalength);
		ramdisk_node->data = new_buffer;
		ramdisk_node->not_on_rd_buffer_length = buflen;
		ramdisk_node->data_on_ramdisk = false;

	} else if (ramdisk_node->data == nullptr) {
		uint32_t initbuflen = 32;
		ramdisk_node->data = new uint8_t[initbuflen];
		ramdisk_node->not_on_rd_buffer_length = initbuflen;
	}

	// when file descriptor shall append, set it to the end
	if (fd->open_flags & G_FILE_FLAG_MODE_APPEND) {
		fd->offset = ramdisk_node->datalength;
	}

	// expand buffer until enough space is available
	uint32_t space;
	while ((space = ramdisk_node->not_on_rd_buffer_length - fd->offset) < length) {

		uint32_t buflen = ramdisk_node->not_on_rd_buffer_length * 1.2;
		uint8_t* new_buffer = new uint8_t[buflen];
		g_memory::copy(new_buffer, ramdisk_node->data, ramdisk_node->datalength);
		delete ramdisk_node->data;
		ramdisk_node->data = new_buffer;
		ramdisk_node->not_on_rd_buffer_length = buflen;
	}

	// copy data
	g_memory::copy(&ramdisk_node->data[fd->offset], buffer(), length);
	ramdisk_node->datalength = fd->offset + length;
	fd->offset += length;

	handler->result = length;
	handler->status = G_FS_WRITE_SUCCESSFUL;
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_write(g_thread* requester, g_fs_write_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_get_length(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_get_length* handler) {

	// the ramdisk handler is doing it's work immediately and doesn't request another process
	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	g_ramdisk_entry* ramdisk_node = g_kernel::ramdisk->findById(node->phys_fs_id);
	if (ramdisk_node == 0) {
		handler->status = G_FS_LENGTH_NOT_FOUND;
		handler->length = 0;
	} else {
		handler->status = G_FS_LENGTH_SUCCESSFUL;
		handler->length = ramdisk_node->datalength;
	}

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_get_length(g_thread* requester, g_fs_transaction_handler_get_length* handler) {

}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_directory_refresh(g_thread* requester, g_fs_node* folder,
		g_fs_transaction_handler_directory_refresh* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	g_ramdisk_entry* rd_folder = g_kernel::ramdisk->findById(folder->phys_fs_id);
	if (rd_folder == 0) {
		handler->status = G_FS_DIRECTORY_REFRESH_ERROR;

	} else {

		// create all nodes that not yet exist
		int position = 0;
		g_ramdisk_entry* rd_child;

		while ((rd_child = g_kernel::ramdisk->getChildAt(folder->phys_fs_id, position++)) != 0) {

			// get real path to parent
			g_local<char> absolute(new char[G_PATH_MAX]);
			g_filesystem::get_real_path_to_node(folder, absolute());

			// append child name
			int abs_cur_len = g_string::length((const char*) absolute());
			int childlen = g_string::length(rd_child->name);
			g_memory::copy(&absolute()[abs_cur_len], "/", 1);
			g_memory::copy(&absolute()[abs_cur_len + 1], rd_child->name, childlen);
			absolute()[abs_cur_len + 1 + childlen] = 0;

			// check if file exists as vfs node
			g_fs_node* fs_childs_parent = 0;
			g_fs_node* fs_child = 0;
			g_local<char> current(new char[G_PATH_MAX]);
			g_filesystem::find_existing(absolute(), &fs_childs_parent, &fs_child, current(), true);

			// if not, create it
			if (fs_child == 0) {
				fs_child = create_vfs_node(rd_child, folder);
			}
		}

		// finish the transaction
		folder->contents_valid = true;
		handler->status = G_FS_DIRECTORY_REFRESH_SUCCESSFUL;
	}

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_directory_refresh(g_thread* requester, g_fs_transaction_handler_directory_refresh* handler) {
}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_open(g_thread* requester, g_fs_node* node, char* filename, int32_t flags, int32_t mode,
		g_fs_transaction_handler_open* handler) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();

	g_ramdisk_entry* ramdisk_node = g_kernel::ramdisk->findById(node->phys_fs_id);

	if (handler->discovery_status == G_FS_DISCOVERY_SUCCESSFUL) {

		if (ramdisk_node->type != G_RAMDISK_ENTRY_TYPE_FILE) {
			g_log_warn("%! only files can be opened, given node ('%s') was a %i", "filesystem", ramdisk_node->name, ramdisk_node->type);
			handler->status = G_FS_OPEN_ERROR;
		} else {
			// truncate file if requested
			if (flags & G_FILE_FLAG_MODE_TRUNCATE) {
				// only applies when data no more used from ramdisk memory
				if (!ramdisk_node->data_on_ramdisk) {
					// completely remove the buffer
					ramdisk_node->datalength = 0;
					ramdisk_node->not_on_rd_buffer_length = 0;
					delete ramdisk_node->data;
					ramdisk_node->data = 0;
				}
			}

			handler->status = G_FS_OPEN_SUCCESSFUL;
		}

	} else if (handler->discovery_status == G_FS_DISCOVERY_NOT_FOUND) {

		if (flags & G_FILE_FLAG_MODE_CREATE) {
			// create the filesystem file
			g_ramdisk_entry* new_ramdisk_entry = g_kernel::ramdisk->createChild(ramdisk_node, filename);

			handler->node = create_vfs_node(new_ramdisk_entry, node);
			handler->status = G_FS_OPEN_SUCCESSFUL;

		} else {
			// return with failure
			handler->status = G_FS_OPEN_NOT_FOUND;
		}

	}

	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_open(g_thread* requester, g_fs_transaction_handler_open* handler) {

}

/**
 *
 */
g_fs_transaction_id g_fs_delegate_ramdisk::request_close(g_thread* requester, g_fs_transaction_handler_close* handler, g_file_descriptor_content* fd,
		g_fs_node* node) {

	g_fs_transaction_id id = g_fs_transaction_store::next_transaction();
	// nothing to do here
	handler->status = G_FS_CLOSE_SUCCESSFUL;
	g_fs_transaction_store::set_status(id, G_FS_TRANSACTION_FINISHED);
	return id;
}

/**
 *
 */
void g_fs_delegate_ramdisk::finish_close(g_thread* requester, g_fs_transaction_handler_close* handler) {

}

