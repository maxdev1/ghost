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

#include "unistd.h"
#include "ghost/kernel.h"
#include "errno.h"

/**
 *
 */
ssize_t read(int fd, void* buf, size_t count) {

	g_fs_read_status stat;
	int32_t len = g_read_s(fd, buf, count, &stat);

	if (stat == G_FS_READ_SUCCESSFUL) {
		return len;

	} else if (stat == G_FS_READ_INVALID_FD) {
		errno = EBADF;

	} else if (stat == G_FS_READ_BUSY) {
		errno = EIO;

	} else {
		// TODO improve kernel error codes
		errno = EIO;

	}

	return -1;
}
