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
#include <stdarg.h>

// redirect
int32_t g_open(const char *name) {
	return g_open_fms(name, 0, 0, 0);
}

// redirect
int32_t g_open_f(const char *name, int32_t flags) {
	return g_open_fms(name, flags, 0, 0);
}

/**
 *
 */
int32_t g_open_fs(const char *name, int32_t flags, g_fs_open_status* out_status) {
	return g_open_fms(name, flags, 0, out_status);
}

/**
 *
 */
int32_t g_open_fms(const char *name, int32_t flags, int32_t mode, g_fs_open_status* out_status) {

	g_syscall_fs_open data;
	data.path = (char*) name;
	data.mode = mode;
	data.flags = flags;
	g_syscall(G_SYSCALL_FS_OPEN, (uint32_t) &data);
	if (out_status) {
		*out_status = data.status;
	}
	return data.fd;
}
