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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_DIRECTORY_REFRESH
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_DIRECTORY_REFRESH

#include "filesystem/fs_transaction_handler.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"

/**
 *
 */
class g_fs_transaction_handler_directory_refresh: public g_fs_transaction_handler {
public:
	g_fs_directory_refresh_status status = G_FS_DIRECTORY_REFRESH_ERROR;
	g_fs_node* folder;
	g_contextual<g_syscall_fs_read_directory*> data;
	g_fs_transaction_handler* unfinished_handler;

	/**
	 *
	 */
	g_fs_transaction_handler_directory_refresh(g_fs_node* folder, g_contextual<g_syscall_fs_read_directory*> bound_data,
			g_fs_transaction_handler* followup_handler) :
			folder(folder), data(bound_data), unfinished_handler(followup_handler) {
	}

	/**
	 *
	 */
	virtual ~g_fs_transaction_handler_directory_refresh() {
		if (unfinished_handler) {
			delete unfinished_handler;
		}
	}

	/**
	 *
	 */
	virtual g_fs_transaction_handler_start_status start_transaction(g_thread* thread);

	/**
	 *
	 */
	virtual g_fs_transaction_handler_finish_status finish_transaction(g_thread* thread, g_fs_delegate* delegate);

};

#endif
