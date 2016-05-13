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

#ifndef GHOST_FILESYSTEM_FILESYSTEMRAMDISKDELEGATE
#define GHOST_FILESYSTEM_FILESYSTEMRAMDISKDELEGATE

#include "utils/hash_map.hpp"
#include "ghost/stdint.h"
#include "memory/contextual.hpp"
#include "filesystem/fs_delegate.hpp"

/**
 *
 */
class g_fs_delegate_ramdisk: public g_fs_delegate {
private:
	/**
	 *
	 */
	g_fs_node* create_vfs_node(g_ramdisk_entry* ramdisk_node, g_fs_node* parent);

public:
	/**
	 *
	 */
	virtual ~g_fs_delegate_ramdisk() {
	}

	/**
	 *
	 */
	virtual g_fs_transaction_id request_discovery(g_thread* requester, g_fs_node* parent, char* child, g_fs_transaction_handler_discovery* handler);

	/**
	 *
	 */
	virtual void finish_discovery(g_thread* requester, g_fs_transaction_handler_discovery* handler);

	/**
	 *
	 */
	virtual g_fs_transaction_id request_read(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer, g_file_descriptor_content* fd,
			g_fs_transaction_handler_read* handler);

	/**
	 *
	 */
	virtual void finish_read(g_thread* requester, g_fs_read_status* out_status, int64_t* out_result, g_file_descriptor_content* fd);

	/**
	 *
	 */
	virtual g_fs_transaction_id request_write(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
			g_file_descriptor_content* fd, g_fs_transaction_handler_write* handler);

	/**
	 *
	 */
	virtual void finish_write(g_thread* requester, g_fs_write_status* out_status, int64_t* out_result, g_file_descriptor_content* fd);

	/**
	 *
	 */
	virtual g_fs_transaction_id request_get_length(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_get_length* handler);

	/**
	 *
	 */
	virtual void finish_get_length(g_thread* requester, g_fs_transaction_handler_get_length* handler);

	/**
	 *
	 */
	virtual g_fs_transaction_id request_directory_refresh(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_directory_refresh* handler);

	/**
	 *
	 */
	virtual void finish_directory_refresh(g_thread* requester, g_fs_transaction_handler_directory_refresh* handler);

	/**
	 *
	 */
	virtual g_fs_transaction_id request_open(g_thread* requester, g_fs_node* node, char* filename, int32_t flags, int32_t mode,
			g_fs_transaction_handler_open* handler);

	/**
	 *
	 */
	virtual void finish_open(g_thread* requester, g_fs_transaction_handler_open* handler);

	/**
	 *
	 */
	virtual g_fs_transaction_id request_close(g_thread* requester, g_fs_transaction_handler_close* handler, g_file_descriptor_content* fd, g_fs_node* node);

	/**
	 *
	 */
	virtual void finish_close(g_thread* requester, g_fs_transaction_handler_close* handler);

};

#endif
