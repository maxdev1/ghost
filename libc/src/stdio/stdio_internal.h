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

#ifndef __GHOST_LIBC_STDIO_INTERNAL__
#define __GHOST_LIBC_STDIO_INTERNAL__

#include "ghost.h"
#include "file.h"

__BEGIN_C

// This file describes non-standard symbols that should not be exposed by the
// public headers and are only used internally.

void __init_stdio();
void __fini_stdio();

// open file list functionality
extern FILE* __open_file_list;
void __open_file_list_add(FILE* file);
void __open_file_list_lock();
void __open_file_list_unlock();

// closes the file, but does not delete the structure
int __fclose_static(FILE* stream);
int __fclose_static_unlocked(FILE* stream);

// parses mode flags
int __parse_mode_flags(const char* mode);

int __fflush_unlocked(FILE* file);
int __fflush_write(FILE* file);
int __fflush_write_unlocked(FILE* file);
int __fflush_read(FILE* file);
int __fflush_read_unlocked(FILE* file);

// same as fopen/fdopen, but on existing FILE structure
FILE* __fopen_static(const char* filename, const char* mode, FILE* file);
int __fdopen_static(int fd, const char *mode, FILE* file);

// applies default buffering to the stream
int __setdefbuf_unlocked(FILE* stream);

size_t __fwrite_unlocked(const void* ptr, size_t size, size_t nmemb,
		FILE* stream);
size_t __fread_unlocked(const void* ptr, size_t size, size_t nmemb,
		FILE* stream);

int __fseeko_unlocked(FILE* stream, off_t offset, int whence);
off_t __ftello_unlocked(FILE* stream);

int __fputc_unlocked(int c, FILE* stream);
int __fgetc_unlocked(FILE* stream);
int __fungetc_unlocked(int c, FILE* stream);

int __setvbuf_unlocked(FILE* stream, char* buf, int mode, size_t size);

int __vfprintf_unlocked(FILE* stream, const char* format, va_list arg);

void __clearerr_unlocked(FILE* stream);

// default implementations, always accessed as "unlocked"
int __stdio_impl_close(FILE* stream);
size_t __stdio_impl_read(void* buf, size_t len, FILE* stream);
size_t __stdio_impl_write(const void* buf, size_t len, FILE* stream);
int __stdio_impl_seek(FILE* stream, off_t offset, int whence);
off_t __stdio_impl_tell(FILE* stream);
int __stdio_impl_fileno(FILE* stream);
int __stdio_impl_close(FILE* stream);
FILE* __stdio_impl_reopen(const char* filename, const char* mode, FILE* stream);
int __stdio_impl_eof(FILE* stream);
int __stdio_impl_error(FILE* stream);
void __stdio_impl_seterr(FILE* stream);
void __stdio_impl_clearerr(FILE* stream);

__END_C

#endif
