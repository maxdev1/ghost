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

#ifndef GHOST_FILESYSTEM_TRANSACTION_HANDLER_GET_LENGTH_SEEK
#define GHOST_FILESYSTEM_TRANSACTION_HANDLER_GET_LENGTH_SEEK

#include "filesystem/fs_transaction_handler_get_length.hpp"
#include "filesystem/fs_node.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "memory/contextual.hpp"
#include "logger/logger.hpp"

/**
 *
 */
class g_fs_transaction_handler_get_length_seek: public g_fs_transaction_handler_get_length {
public:
	g_file_descriptor_content* fd;
	g_contextual<g_syscall_fs_seek*> data;

	/**
	 *
	 */
	g_fs_transaction_handler_get_length_seek(g_file_descriptor_content* fd, g_fs_node* node, g_contextual<g_syscall_fs_seek*> data) :
			g_fs_transaction_handler_get_length(node), fd(fd), data(data) {
	}

	/**
	 *
	 */
	virtual void perform_afterwork(g_thread* thread) {

		if (status == G_FS_LENGTH_SUCCESSFUL) {

			// add amount to offset
			if (data()->mode == G_FS_SEEK_CUR) {
				fd->offset += data()->amount;
			} else if (data()->mode == G_FS_SEEK_SET) {
				fd->offset = data()->amount;
			} else if (data()->mode == G_FS_SEEK_END) {
				fd->offset = length - data()->amount;
			}

			// validate offset
			if (fd->offset > length) {
				fd->offset = length;
			}
			if (fd->offset < 0) {
				fd->offset = 0;
			}

			data()->result = fd->offset;
			data()->status = G_FS_SEEK_SUCCESSFUL;

		} else {
			data()->result = -1;
			data()->status = G_FS_SEEK_ERROR;
		}
	}

};

#endif
