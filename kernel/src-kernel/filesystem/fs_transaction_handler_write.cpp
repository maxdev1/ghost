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

#include "filesystem/fs_delegate.hpp"
#include "filesystem/fs_transaction_handler_write.hpp"
#include "filesystem/filesystem.hpp"

/**
 *
 */
g_fs_transaction_handler_start_status g_fs_transaction_handler_write::start_transaction(g_thread* thread) {

	g_contextual<uint8_t*> contbuf(data()->buffer, thread->process->pageDirectory);
	return g_filesystem::write(thread, node, fd, data()->length, contbuf, this);
}

/**
 *
 */
g_fs_transaction_handler_status g_fs_transaction_handler_write::finish_transaction(g_thread* thread, g_fs_delegate* delegate) {
	delegate->finish_write(thread, &status, &result, fd);
	data()->result = result;
	data()->status = status;
	return G_FS_TRANSACTION_HANDLING_DONE;
}
