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

#ifndef __GHOST_LIBC_FILE__
#define __GHOST_LIBC_FILE__

#include "ghost/common.h"
#include "ghost/fs.h"
#include "sys/types.h"
#include "stdint.h"

__BEGIN_C

#define G_FILE_UNGET_PRESERVED_SPACE		4			// space preserved for ungetc-calls

#define G_FILE_FLAG_EOF						(1 << 26)	// end of file reached
#define G_FILE_FLAG_ERROR					(1 << 27)	// when a file error has occured
#define G_FILE_FLAG_BUFFER_SET				(1 << 28)	// whether the buffer was ever set
#define G_FILE_FLAG_BUFFER_DIRECTION_READ	(1 << 29)	// last access was read
#define G_FILE_FLAG_BUFFER_DIRECTION_WRITE	(1 << 30)	// last access was write
#define G_FILE_FLAG_BUFFER_OWNER_LIBRARY	(1 << 31)	// buffer is owned by the library (was created in setvbuf for example)

typedef struct FILE FILE;

/**
 * Represents a stream. Used by stdio-related functions.
 * (N1548-7.21.1-2)
 */
struct FILE {
	g_fd file_descriptor;
	g_user_mutex lock;

	uint8_t* buffer;
	size_t buffer_size;
	uint8_t buffer_mode;
	size_t buffered_bytes_write;
	size_t buffered_bytes_read;
	size_t buffered_bytes_read_offset;

	ssize_t (*impl_read)(void* buf, size_t len, FILE* stream);
	ssize_t (*impl_write)(const void* buf, size_t len, FILE* stream);
	int (*impl_seek)(FILE*, off_t, int);
	off_t (*impl_tell)(FILE*);
	int (*impl_close)(FILE*);
	FILE* (*impl_reopen)(const char*, const char*, FILE*);
	int (*impl_fileno)(FILE*);
	int (*impl_eof)(FILE*);
	int (*impl_error)(FILE*);
	void (*impl_clearerr)(FILE*);
	void (*impl_seterr)(FILE*);

	uint32_t flags;

	FILE* prev;
	FILE* next;
};

__END_C

#endif
