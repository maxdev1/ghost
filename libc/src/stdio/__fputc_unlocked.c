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
int __fputc_unlocked(int c, FILE* stream) {

	uint8_t c8 = (uint8_t) c;

	// if necessary, initialize stream buffer
	if ((stream->flags & G_FILE_FLAG_BUFFER_SET) == 0) {
		if (__setdefbuf_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	// for unbuffered streams, perform direct write
	if (stream->buffer_mode == _IONBF) {

		if (__fwrite_unlocked(&c8, 1, 1, stream) != 1) {
			return EOF;
		}
		return c8;
	}

	// if the last access was a read, flush it
	if (stream->flags & G_FILE_FLAG_BUFFER_DIRECTION_READ) {
		if (__fflush_read_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	// check if buffer is full
	if (stream->buffered_bytes_write == stream->buffer_size) {
		if (__fflush_write_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	// set direction
	stream->flags |= G_FILE_FLAG_BUFFER_DIRECTION_WRITE;

	// put byte into buffer
	stream->buffer[stream->buffered_bytes_write++] = c8;

	// flush stream if its line-buffered and a newline occurs
	if (stream->buffer_mode == _IOLBF && c8 == '\n') {
		if (__fflush_write_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	return c8;
}
