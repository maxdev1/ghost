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
#include "fcntl.h"
#include "errno.h"
#include <stdarg.h>

/**
 *
 */
int open(const char* pathname, int flags, ...) {

	mode_t mode = 0;
	g_fs_open_status status;

	// if the create flag is given, we must have a mode
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

	// perform opening syscall
	g_fd fd = g_open_fs(pathname, flags, &status);

	if (status == G_FS_OPEN_SUCCESSFUL) {
		return fd;
	} else if (status == G_FS_OPEN_NOT_FOUND) {
		errno = ENOENT;
	} else {
		// TODO set error codes, improve kernel error codes
		errno = EIO;
	}

	return -1;
}
