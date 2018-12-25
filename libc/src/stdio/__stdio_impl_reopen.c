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
FILE* __stdio_impl_reopen(const char* filename, const char* mode,
		FILE* stream) {

	// if no filename specified, attempt to change file mode
	if (filename == NULL) {

		// parse mode flags
		int newmode = __parse_mode_flags(mode);
		if (newmode == EOF) {
			return NULL;
		}

		// TODO ask kernel to change the flags
		stream->flags &= ~G_FILE_FLAGS_MODE_RANGE;
		stream->flags |= newmode;
		return stream;
	}

	// close file
	int close = __fclose_static_unlocked(stream);
	if (close != 0) {
		return NULL;
	}

	// open again
	return __fopen_static(filename, mode, stream);
}
