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
int __fungetc_unlocked(int c, FILE* stream) {

	// if necessary, initialize stream buffer
	if ((stream->flags & G_FILE_FLAG_BUFFER_SET) == 0) {
		if (__setdefbuf_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	// unbuffered streams currently don't support this
	if (stream->buffer_mode == _IONBF) {
		errno = EBADF;
		stream->flags |= G_FILE_FLAG_ERROR;
		return EOF;
	}

	// if the last access was a write, flush it
	if (stream->flags & G_FILE_FLAG_BUFFER_DIRECTION_WRITE) {
		if (__fflush_write_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	// set direction
	stream->flags |= G_FILE_FLAG_BUFFER_DIRECTION_READ;

	// ungetting makes stream no longer EOF
	stream->flags &= ~G_FILE_FLAG_EOF;

	// call with EOF results in nothing
	if (c == EOF) {
		return EOF;
	}

	// put byte back on stream buffer
	uint8_t c8 = (uint8_t) c;

	// if we reached left end of buffer, try to move bytes
	if (stream->buffered_bytes_read_offset == 0) {
		size_t buffered_bytes = stream->buffered_bytes_read
				- stream->buffered_bytes_read_offset;
		size_t buffer_space = stream->buffer_size - buffered_bytes;

		if (buffer_space == 0) {
			// can't move buffer content, this fails
			return EOF;
		}

		// move bytes to right
		memmove(stream->buffer + buffer_space, stream->buffer,
				sizeof(stream->buffer[0]) * buffered_bytes);
		stream->buffered_bytes_read_offset = buffer_space;
		stream->buffered_bytes_read = buffer_space + buffered_bytes;
	}

	stream->buffer[--stream->buffered_bytes_read_offset] = c8;
	return c8;
}
