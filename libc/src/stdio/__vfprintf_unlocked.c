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
static ssize_t vcbprintf_vfprintf_callback(void* param, const char* string,
		size_t stringlen) {

	return __fwrite_unlocked(string, 1, stringlen, (FILE*) param);
}

/**
 *
 */
int __vfprintf_unlocked(FILE* stream, const char* format, va_list arg) {
	if ((stream->flags & G_FILE_FLAG_MODE_WRITE) == 0) {
		errno = EBADF;
		stream->flags |= G_FILE_FLAG_ERROR;
		return EOF;
	}

	return vcbprintf(stream, vcbprintf_vfprintf_callback, format, arg);
}
