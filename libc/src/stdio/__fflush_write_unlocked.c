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
int __fflush_write_unlocked(FILE* stream) {

	// if nothing was written to the stream, return
	if ((stream->flags & G_FILE_FLAG_BUFFER_DIRECTION_WRITE) == 0) {
		return 0;
	}

	// if stream is not writable, return in error
	if ((stream->flags & G_FILE_FLAG_MODE_WRITE) == 0) {
		errno = EBADF;
		stream->flags |= G_FILE_FLAG_ERROR;
		return EOF;
	}

	// if stream has no write implementation, return with error
	if (stream->impl_write == NULL) {
		errno = EBADF;
		stream->flags |= G_FILE_FLAG_ERROR;
		return EOF;
	}

	// perform writing
	size_t res = 0;

	size_t total = stream->buffered_bytes_write;
	size_t done = 0;

	while (done < total) {
		// call write implementation
		ssize_t written = stream->impl_write((void*) (stream->buffer + done),
				total - done, stream);

		if (written == 0) {
			stream->flags |= G_FILE_FLAG_EOF;
			res = EOF;
			break;

		} else if (written == -1) {
			stream->flags |= G_FILE_FLAG_ERROR;
			res = EOF;
			break;
		}

		done += written;
	}

	// stream has no direction anymore
	stream->flags &= ~G_FILE_FLAG_BUFFER_DIRECTION_WRITE;

	// all buffered bytes are written
	stream->buffered_bytes_write = 0;

	return res;
}
