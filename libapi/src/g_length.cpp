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
#include <string.h>

// redirect
int64_t g_length(g_fd fd) {
	return g_length_s(fd, 0);
}

/**
 *
 */
int64_t g_length_s(g_fd fd, g_fs_length_status* out_status) {

	g_syscall_fs_length data;
	data.mode = G_SYSCALL_FS_LENGTH_BY_FD;
	data.fd = fd;
	g_syscall(G_SYSCALL_FS_LENGTH, (uint32_t) &data);
	if (out_status) {
		*out_status = data.status;
	}
	return data.length;
}

// redirect
int64_t g_flength(const char* path) {

	return g_flength_ss(path, true, 0);
}

// redirect
int64_t g_flength_s(const char* path, uint8_t follow_symlinks) {

	return g_flength_ss(path, follow_symlinks, 0);
}

/**
 *
 */
int64_t g_flength_ss(const char* path, uint8_t follow_symlinks, g_fs_length_status* out_status) {

	g_syscall_fs_length data;
	int symlink_flag = (follow_symlinks ? G_SYSCALL_FS_LENGTH_FOLLOW_SYMLINKS : G_SYSCALL_FS_LENGTH_NOT_FOLLOW_SYMLINKS);
	data.mode = symlink_flag | G_SYSCALL_FS_LENGTH_BY_PATH;
	data.path = (char*) path;
	g_syscall(G_SYSCALL_FS_LENGTH, (uint32_t) &data);
	if (out_status) {
		*out_status = data.status;
	}
	return data.length;
}

