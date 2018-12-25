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
#include "malloc.h"

/**
 *
 */
int __setvbuf_unlocked(FILE* stream, char* buf, int mode, size_t size) {

	// free old buffer if library is owner
	if ((stream->buffer_mode != _IONBF) && (stream->buffer != NULL)
			&& (stream->flags & G_FILE_FLAG_BUFFER_OWNER_LIBRARY)) {
		free(stream->buffer);
	}

	// create buffer if necessary
	if (buf == NULL) {
		// normalize buffer size
		if (size == 0) {
			size = BUFSIZ;
		} else if (size < BUFSIZMIN) {
			size = BUFSIZMIN;
		}

		// allocate buffer memory
		buf = (char*) malloc(size);
		if (buf == NULL) {
			return -1;
		}

		// set library as buffer owner
		stream->flags |= G_FILE_FLAG_BUFFER_OWNER_LIBRARY;
	} else {
		// set user as buffer owner
		stream->flags &= ~G_FILE_FLAG_BUFFER_OWNER_LIBRARY;
	}

	// set fields
	stream->buffer = (uint8_t*) buf;
	stream->buffer_mode = mode;
	stream->buffer_size = size;
	stream->buffered_bytes_read = 0;
	stream->buffered_bytes_read_offset = 0;
	stream->buffered_bytes_write = 0;
	stream->flags |= G_FILE_FLAG_BUFFER_SET;

	return 0;
}
