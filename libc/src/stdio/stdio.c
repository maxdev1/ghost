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

#define _DEFAULT_BUFSIZE	1024

FILE _stdin;
FILE* stdin = &_stdin;
char _stdin_buf[_DEFAULT_BUFSIZE];

FILE _stdout;
FILE* stdout = &_stdout;
char _stdout_buf[_DEFAULT_BUFSIZE];

FILE _stderr;
FILE* stderr = &_stderr;
char _stderr_buf[_DEFAULT_BUFSIZE];

/**
 *
 */
void __init_stdio() {

	// this initialization method avoids the use of malloc in the early
	// stage and leaves the task of allocating enough space to the OS,
	// allowing the program to fail on load instead of here, where it
	// could not be handled properly

	memset(stdin, 0, sizeof(FILE));
	__fdopen_static(STDIN_FILENO, "r", stdin);
	setvbuf(stdin, _stdin_buf, _IOLBF, _DEFAULT_BUFSIZE);

	memset(stdout, 0, sizeof(FILE));
	__fdopen_static(STDOUT_FILENO, "w", stdout);
	setvbuf(stdout, _stdout_buf, _IOLBF, _DEFAULT_BUFSIZE);

	memset(stderr, 0, sizeof(FILE));
	__fdopen_static(STDERR_FILENO, "w", stderr);
	setvbuf(stderr, _stderr_buf, _IONBF, _DEFAULT_BUFSIZE);
}

/**
 *
 */
void __fini_stdio() {

	// close all descriptors
	// skip stdin/stdout/stderr
	FILE* f = __open_file_list;
	while (f) {
		FILE* n = f->next;
		if(f->file_descriptor > STDERR_FILENO && g_atomic_try_lock(&f->lock)) {
			__fclose_static_unlocked(f);
			f->lock = 0;
		}
		f = n;
	}
}

