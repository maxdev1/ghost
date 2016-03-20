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
#include "string.h"
#include "errno.h"

/**
 *
 */
int __parse_mode_flags(const char* mode) {

	int flags = 0;

	switch (*mode) {
	case 'r':
		flags = G_FILE_FLAG_MODE_READ;
		break;
	case 'w':
		flags = G_FILE_FLAG_MODE_WRITE | G_FILE_FLAG_MODE_CREATE
				| G_FILE_FLAG_MODE_TRUNCATE;
		break;
	case 'a':
		flags = G_FILE_FLAG_MODE_WRITE | G_FILE_FLAG_MODE_CREATE
				| G_FILE_FLAG_MODE_APPEND;
		break;
	default:
		errno = EINVAL;
		return EOF;
	}

	while (*(++mode)) {
		switch (*mode) {
		case '+':
			flags |= G_FILE_FLAG_MODE_READ | G_FILE_FLAG_MODE_WRITE;
			break;
		case 't':
			flags &= ~G_FILE_FLAG_MODE_BINARY;
			flags |= G_FILE_FLAG_MODE_TEXTUAL;
			break;
		case 'b':
			flags &= ~G_FILE_FLAG_MODE_TEXTUAL;
			flags |= G_FILE_FLAG_MODE_BINARY;
			break;
		default:
			errno = EINVAL;
			return EOF;
		}
	}

	return flags;
}
