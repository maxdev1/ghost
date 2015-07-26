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

#ifndef GHOST_FILESYSTEM_FILESYSTEMTRANSACTIONSTORE
#define GHOST_FILESYSTEM_FILESYSTEMTRANSACTIONSTORE

#include "filesystem/fs_node.hpp"
#include "memory/paging.hpp"

/**
 * Address-space bound meta object passed during transactions.
 */
struct g_fs_transaction_store_meta {
	void* value;
	g_page_directory space;
};

/**
 * The discovery store is used to store information about an ongoing discovery.
 * A discovery happens when a child for a specific node should be resolved by a driver
 * delegate which must happen asynchronously.
 */
class g_fs_transaction_store {
public:
	static void initialize();

	static g_fs_transaction_id next_transaction();
	static void set_status(g_fs_transaction_id id, g_fs_transaction_status result);
	static g_fs_transaction_status get_status(g_fs_transaction_id id);
	static void remove_transaction(g_fs_transaction_id id);
};

#endif
