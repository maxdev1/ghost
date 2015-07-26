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

// redirect
int64_t g_seek(g_fd fd, int64_t off, g_fs_seek_mode mode) {
	return g_seek_s(fd, off, mode, 0);
}

/**
 *
 */
int64_t g_seek_s(g_fd fd, int64_t off, g_fs_seek_mode mode, g_fs_seek_status* out_status) {

	g_syscall_fs_seek data;
	data.fd = fd;
	data.amount = off;
	data.mode = mode;
	g_syscall(G_SYSCALL_FS_SEEK, (uint32_t) &data);
	if (out_status) {
		*out_status = data.status;
	}
	return data.result;

}
