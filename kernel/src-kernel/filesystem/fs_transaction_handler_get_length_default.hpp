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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_GET_LENGTH_DEFAULT
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_GET_LENGTH_DEFAULT

#include "filesystem/fs_transaction_handler_get_length.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"
#include "logger/logger.hpp"

/**
 *
 */
class g_fs_transaction_handler_get_length_default: public g_fs_transaction_handler_get_length {
public:
	g_contextual<g_syscall_fs_length*> data;

	/**
	 *
	 */
	g_fs_transaction_handler_get_length_default(g_contextual<g_syscall_fs_length*> data, g_fs_node* node) :
			g_fs_transaction_handler_get_length(node), data(data) {
	}

	/**
	 *
	 */
	virtual void perform_afterwork(g_thread* thread) {

		if (status == G_FS_LENGTH_SUCCESSFUL) {
			data()->status = G_FS_LENGTH_SUCCESSFUL;
			data()->length = length;

		} else {
			data()->status = status;
			data()->length = -1;
		}
	}

};

#endif
