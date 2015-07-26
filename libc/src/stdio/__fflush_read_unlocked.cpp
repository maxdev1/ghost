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
#include "errno.h"

/**
 *
 */
int __fflush_read_unlocked(FILE* stream) {

	// if nothing was read from the stream, return
	if ((stream->flags & G_FILE_FLAG_BUFFER_DIRECTION_READ) == 0) {
		return 0;
	}

	// if stream is not readable, return in error
	if ((stream->flags & G_FILE_FLAG_MODE_READ) == 0) {
		errno = EBADF;
		stream->flags |= G_FILE_FLAG_ERROR;
		return EOF;
	}

	// get current location in file (must preserve errno)
	int preserved_errno = errno;
	off_t current_position = -1;
	if (stream->impl_seek) {
		current_position = stream->impl_seek(stream, 0, SEEK_CUR);
	}
	errno = preserved_errno;

	// restore the position
	int res = 0;
	if (current_position >= 0) {
		size_t buffered_bytes = stream->buffered_bytes_read
				- stream->buffered_bytes_read_offset;
		off_t restored_position =
				(uintmax_t) current_position < (uintmax_t) buffered_bytes ?
						0 : current_position - buffered_bytes;

		if (stream->impl_seek(stream, restored_position, SEEK_SET) < 0) {
			stream->flags |= G_FILE_FLAG_ERROR;
		}
	}

	// reset buffer fields
	stream->buffered_bytes_read = 0;
	stream->buffered_bytes_read_offset = 0;

	// reset direction
	stream->flags &= ~G_FILE_FLAG_BUFFER_DIRECTION_READ;

	return res;
}
