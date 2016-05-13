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

#ifndef GHOST_FILESYSTEM_FILESYSTEMDELEGATE
#define GHOST_FILESYSTEM_FILESYSTEMDELEGATE

#include "ghost/stdint.h"
#include "utils/hash_map.hpp"
#include "filesystem/fs_transaction_store.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "filesystem/fs_transaction_handler.hpp"
#include "filesystem/fs_transaction_handler_read.hpp"
#include "filesystem/fs_transaction_handler_read_directory.hpp"
#include "filesystem/fs_transaction_handler_directory_refresh.hpp"
#include "filesystem/fs_transaction_handler_write.hpp"
#include "filesystem/fs_transaction_handler_discovery.hpp"
#include "filesystem/fs_transaction_handler_get_length.hpp"
#include "filesystem/fs_transaction_handler_open.hpp"
#include "filesystem/fs_transaction_handler_close.hpp"
#include "memory/contextual.hpp"

/**
 *
 */
class g_fs_delegate {
public:
	/**
	 *
	 */
	virtual ~g_fs_delegate() {
	}

	/**
	 * Performs a discovery transaction (this means that a lookup is made to find an
	 * actual file with the given name) for a name, searching with a specific parent node.
	 *
	 * @param parent
	 * 		the parent node
	 * @param child
	 * 		the child to discover
	 * @param handler
	 * 		the finish handler
	 *
	 * @return the transaction id
	 */
	virtual g_fs_transaction_id request_discovery(g_thread* requester, g_fs_node* parent, char* child, g_fs_transaction_handler_discovery* handler) = 0;

	/**
	 * Finishs a read transaction, allowing the delegate implementation to fill the
	 * output status.
	 *
	 * @param requester
	 * 		the thread requesting read
	 * @param handler
	 * 		the finish handler
	 */
	virtual void finish_discovery(g_thread* requester, g_fs_transaction_handler_discovery* handler) = 0;

	/**
	 * Performs a read transaction.
	 *
	 * @param requester
	 * 		the thread requesting read
	 * @param node
	 * 		the node to read from
	 * @param fd
	 * 		the file descriptor
	 * @param handler
	 * 		the finish handler
	 *
	 * @return the transaction id
	 */
	virtual g_fs_transaction_id request_read(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer, g_file_descriptor_content* fd,
			g_fs_transaction_handler_read* handler) = 0;

	/**
	 * Finishs a read transaction, allowing the delegate implementation to fill
	 * results into the call data struct.
	 *
	 * @param requester
	 * 		the thread requesting read
	 * @param data
	 * 		the call data to fill
	 * @param fd
	 * 		the file descriptor that was read from
	 */
	virtual void finish_read(g_thread* requester, g_fs_read_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) = 0;

	/**
	 * Performs a write transaction.
	 *
	 * @param requester
	 * 		the thread requesting write
	 * @param node
	 * 		the node to write from
	 * @param fd
	 * 		the file descriptor
	 * @param handler
	 * 		the finish handler
	 *
	 * @return the transaction id
	 */
	virtual g_fs_transaction_id request_write(g_thread* requester, g_fs_node* node, int64_t length, g_contextual<uint8_t*> buffer,
			g_file_descriptor_content* fd, g_fs_transaction_handler_write* handler) = 0;

	/**
	 * Finishs a write transaction, allowing the delegate implementation to fill
	 * results into the call data struct.
	 *
	 * @param requester
	 * 		the thread requesting read
	 * @param data
	 * 		the call data to fill
	 * @param fd
	 * 		the file descriptor that was read from
	 */
	virtual void finish_write(g_thread* requester, g_fs_write_status* out_status, int64_t* out_result, g_file_descriptor_content* fd) = 0;

	/**
	 * Performs a get-length transaction
	 *
	 * @param requester
	 * 		the thread requesting get-length
	 * @param node
	 * 		the node to get length of
	 * @param handler
	 * 		the finish handler
	 *
	 * @return the transaction id
	 */
	virtual g_fs_transaction_id request_get_length(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_get_length* handler) = 0;

	/**
	 * Finishs a get-length transaction, allowing the delegate implementation to fill
	 * results into the requested variable.
	 *
	 * @param requester
	 * 		the thread requesting read
	 * @param handler
	 * 		the finish handler
	 */
	virtual void finish_get_length(g_thread* requester, g_fs_transaction_handler_get_length* handler) = 0;

	/**
	 * Refreshing the directory means that the executing delegate shall create all VFS nodes for
	 * all children of the given node.
	 */
	virtual g_fs_transaction_id request_directory_refresh(g_thread* requester, g_fs_node* node, g_fs_transaction_handler_directory_refresh* handler) = 0;

	/**
	 * Finishes the directory refresh transaction, allowing the delegate to fill the results into
	 * the output
	 */
	virtual void finish_directory_refresh(g_thread* requester, g_fs_transaction_handler_directory_refresh* handler) = 0;

	/**
	 * Used to open a file. The given node can be two things here: If the node was found (meaning the file
	 * already exists) it is the node itself. Otherwise it is the last discovered parent node and the filename
	 * is the name of the file that shall be created.
	 */
	virtual g_fs_transaction_id request_open(g_thread* requester, g_fs_node* node, char* filename, int32_t flags, int32_t mode,
			g_fs_transaction_handler_open* handler) = 0;

	/**
	 *
	 */
	virtual void finish_open(g_thread* requester, g_fs_transaction_handler_open* handler) = 0;

	/**
	 * Used to close a file.
	 */
	virtual g_fs_transaction_id request_close(g_thread* requester, g_fs_transaction_handler_close* handler, g_file_descriptor_content* fd, g_fs_node* node) = 0;

	/**
	 *
	 */
	virtual void finish_close(g_thread* requester, g_fs_transaction_handler_close* handler) = 0;

};

#endif
