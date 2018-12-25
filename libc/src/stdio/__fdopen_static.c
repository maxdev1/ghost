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

#include "stdio.h"
#include "stdio_internal.h"
#include "stdint.h"
#include "file.h"
#include "unistd.h"
#include "stdlib.h"

/**
 *
 */
int __fdopen_static(int fd, const char* mode, FILE* file) {

	// parse mode flags
	int flags = __parse_mode_flags(mode);
	if (flags == EOF) {
		return EOF;
	}

	// set file descriptor and flags
	file->file_descriptor = fd;
	file->flags = flags;

	// not buffered
	file->buffer_mode = _IONBF;

	// default implementations
	file->impl_close = __stdio_impl_close;
	file->impl_read = __stdio_impl_read;
	file->impl_write = __stdio_impl_write;
	file->impl_seek = __stdio_impl_seek;
	file->impl_tell = __stdio_impl_tell;
	file->impl_fileno = __stdio_impl_fileno;
	file->impl_reopen = __stdio_impl_reopen;
	file->impl_error = __stdio_impl_error;
	file->impl_eof = __stdio_impl_eof;

	// add file to list of open files
	__open_file_list_add(file);

	return 0;
}
