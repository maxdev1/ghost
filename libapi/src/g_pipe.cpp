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
#include "ghost/stdint.h"

// redirect
void g_pipe(g_fd* out_write, g_fd* out_read) {
	return g_pipe_bs(out_write, out_read, true, 0);
}

void g_pipe_s(g_fd* out_write, g_fd* out_read, g_fs_pipe_status* out_status) {
	return g_pipe_bs(out_write, out_read, true, out_status);
}

void g_pipe_b(g_fd* out_write, g_fd* out_read, g_bool blocking) {
	return g_pipe_bs(out_write, out_read, blocking, 0);
}

/**
 *
 */
void g_pipe_bs(g_fd* out_write, g_fd* out_read, g_bool blocking, g_fs_pipe_status* out_status) {

	g_syscall_fs_pipe data;
	data.blocking = blocking;

	g_syscall(G_SYSCALL_FS_PIPE, (uint32_t) &data);
	*out_write = data.write_fd;
	*out_read = data.read_fd;
	if (out_status) {
		*out_status = data.status;
	}
}
