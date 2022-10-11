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

#include "ghost.h"
#include <stdarg.h>

// redirect
g_fd g_clone_fd(g_fd source_fd, g_pid source_process, g_pid target_process) {
	return g_clone_fd_ts(source_fd, source_process, -1, target_process, 0);
}

// redirect
g_fd g_clone_fd_s(g_fd source_fd, g_pid source_process, g_pid target_process, g_fs_clonefd_status* out_status) {
	return g_clone_fd_ts(source_fd, source_process, -1, target_process, out_status);
}

// redirect
g_fd g_clone_fd_t(g_fd source_fd, g_pid source_process, g_fd target_fd, g_pid target_process) {
	return g_clone_fd_ts(source_fd, source_process, target_fd, target_process, 0);
}

/**
 *
 */
g_fd g_clone_fd_ts(g_fd source_fd, g_pid source_process, g_fd target_fd, g_pid target_process, g_fs_clonefd_status* out_status) {

	g_syscall_fs_clonefd data;
	data.source_fd = source_fd;
	data.source_pid = source_process;
	data.target_fd = target_fd;
	data.target_pid = target_process;
	g_syscall(G_SYSCALL_FS_CLONEFD, (g_address) &data);
	if (out_status) {
		*out_status = data.status;
	}
	return data.result;
}
