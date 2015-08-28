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

/**
 *
 */
int __fgetc_unlocked(FILE* stream) {

	// if necessary, initialize stream buffer
	if ((stream->flags & G_FILE_FLAG_BUFFER_SET) == 0) {
		if (__setdefbuf_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	// for unbuffered streams, perform direct write
	if (stream->buffer_mode == _IONBF) {

		uint8_t c8;
		if (__fread_unlocked(&c8, 1, 1, stream) != 1) {
			return EOF;
		}
		return c8;
	}

	// if the last access was a write, flush it
	if (stream->flags & G_FILE_FLAG_BUFFER_DIRECTION_WRITE) {
		if (__fflush_write_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	// set direction
	stream->flags |= G_FILE_FLAG_BUFFER_DIRECTION_READ;

	// check if buffer is empty
	if (stream->buffered_bytes_read_offset >= stream->buffered_bytes_read) {

		// keep some space for ungetc-calls to avoid moving memory
		size_t unget_space = G_FILE_UNGET_PRESERVED_SPACE;

		// if buffer is too small, leave no space
		if (unget_space >= stream->buffer_size) {
			unget_space = 0;
		}

		// fill buffer with data
		ssize_t read = stream->impl_read(stream->buffer + unget_space,
				stream->buffer_size - unget_space, stream);

		if (read == 0) {
			stream->flags |= G_FILE_FLAG_EOF;
			return EOF;

		} else if (read == -1) {
			stream->flags |= G_FILE_FLAG_ERROR;
			return EOF;
		}

		// set buffer fields
		stream->buffered_bytes_read = unget_space + read;
		stream->buffered_bytes_read_offset = unget_space;
	}

	return stream->buffer[stream->buffered_bytes_read_offset++];
}
