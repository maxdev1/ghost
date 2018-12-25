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
#include "fcntl.h"

/**
 *
 */
FILE* __fopen_static(const char* filename, const char* mode, FILE* file) {

	// open file
	int flags = __parse_mode_flags(mode);
	if (flags == EOF) {
		return NULL;
	}

	// perform actual open
	int fd = open(filename, flags);
	if (fd == -1) {
		return NULL;
	}

	// create file handle
	if (__fdopen_static(fd, mode, file) == 0) {
		return file;
	}

	// if it failed, close file
	close(fd);
	return NULL;
}
