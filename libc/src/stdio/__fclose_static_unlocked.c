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
#include "malloc.h"
#include "file.h"
#include "unistd.h"
#include "string.h"

/**
 *
 */
int __fclose_static_unlocked(FILE* stream) {

	// flush stream
	__fflush_unlocked(stream);

	// close file descriptor
	if (stream->impl_close(stream)) {
		return EOF;
	}

	// if library created buffer, delete it
	if ((stream->flags & G_FILE_FLAG_BUFFER_SET) && (stream->buffer != NULL)
			&& (stream->flags & G_FILE_FLAG_BUFFER_OWNER_LIBRARY)) {
		free(stream->buffer);
	}

	// reset contents
	memset(stream, 0, sizeof(FILE));

	// remove from open file list
	__open_file_list_remove(stream);

	return 0;
}
