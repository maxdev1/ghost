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
off_t lseek(int fd, off_t offset, int whence) {

	g_fs_seek_mode mode;
	if (whence == SEEK_SET) {
		mode = G_FS_SEEK_SET;
	} else if (whence == SEEK_CUR) {
		mode = G_FS_SEEK_CUR;
	} else if (whence == SEEK_END) {
		mode = G_FS_SEEK_END;
	}

	g_fs_seek_status status;
	int64_t result = g_seek_s(fd, offset, mode, &status);

	if (status == G_FS_SEEK_SUCCESSFUL) {
		return result;

	} else if (status == G_FS_SEEK_INVALID_FD) {
		errno = EBADF;

	} else {
		// TODO improve kernel error codes
		errno = EIO;

	}

	return -1;
}

