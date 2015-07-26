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

#include "ghost/user.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

// redirect
g_fs_directory_entry* g_read_directory(g_fs_directory_iterator* iterator) {
	return g_read_directory_s(iterator, 0);
}

/**
 *
 */
g_fs_directory_entry* g_read_directory_s(g_fs_directory_iterator* iterator, g_fs_read_directory_status* out_status) {

	g_syscall_fs_read_directory data;
	data.iterator = iterator;
	g_syscall(G_SYSCALL_FS_READ_DIRECTORY, (uint32_t) &data);

	if (out_status) {
		*out_status = data.status;
	}

	if (data.status == G_FS_READ_DIRECTORY_SUCCESSFUL) {
		return &iterator->entry_buffer;
	}

	return 0;
}
