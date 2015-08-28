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
size_t __fread_unlocked(const void* ptr, size_t size, size_t nmemb,
		FILE* stream) {

	// check for illegal arguments
	if (size == 0 || nmemb == 0) {
		return 0;
	}

	// check if stream is writable
	if ((stream->flags & G_FILE_FLAG_MODE_READ) == 0) {
		errno = EBADF;
		stream->flags |= G_FILE_FLAG_ERROR;
		return EOF;
	}

	// if necessary, initialize stream buffer
	if ((stream->flags & G_FILE_FLAG_BUFFER_SET) == 0) {
		if (__setdefbuf_unlocked(stream) == EOF) {
			return EOF;
		}
	}

	// unbuffered files perform direct read
	if (stream->buffer_mode == _IONBF) {

		// if the last access was a write, flush it
		if (stream->flags & G_FILE_FLAG_BUFFER_DIRECTION_WRITE) {
			if (__fflush_write_unlocked(stream) == EOF) {
				return EOF;
			}
		}

		// if stream has no read implementation, return with error
		if (stream->impl_read == NULL) {
			errno = EBADF;
			stream->flags |= G_FILE_FLAG_ERROR;
			return EOF;
		}

		// set stream direction
		stream->flags |= G_FILE_FLAG_BUFFER_DIRECTION_READ;

		// remove end-of-file
		stream->flags &= ~G_FILE_FLAG_EOF;

		// perform reading
		size_t res = 0;

		size_t total = size * nmemb;
		size_t done = 0;

		while (done < total) {
			// call read implementation
			ssize_t read = stream->impl_read(&(((uint8_t*) ptr)[done]), total,
					stream);

			if (read == 0) {
				stream->flags |= G_FILE_FLAG_EOF;
				res = EOF;
				break;

			} else if (read == -1) {
				stream->flags |= G_FILE_FLAG_ERROR;
				res = EOF;
				break;
			}

			done += read;
		}

		return done / size;
	}

	// for buffered streams, get char-by-char
	uint8_t* buffer = (uint8_t*) ptr;

	for (size_t e = 0; e < nmemb; e++) {
		size_t off = e * size;
		for (size_t b = 0; b < size; b++) {
			int c = __fgetc_unlocked(stream);
			if (c == EOF) {
				return e;
			}
			buffer[off + b] = (uint8_t) c;
		}
	}

	return nmemb;
}
