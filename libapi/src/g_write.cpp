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
int32_t g_write(g_fd file, const void* buffer, uint64_t length) {
	return g_write_s(file, buffer, length, 0);
}

/**
 *
 */
int32_t g_write_s(g_fd file, const void* buffer, uint64_t length, g_fs_write_status* out_status) {

	g_syscall_fs_write data;
	data.fd = file;
	data.buffer = (uint8_t*) buffer;
	data.length = length;
	g_syscall(G_SYSCALL_FS_WRITE, (uint32_t) &data);
	if (out_status) {
		*out_status = data.status;
	}
	return data.result;
}
