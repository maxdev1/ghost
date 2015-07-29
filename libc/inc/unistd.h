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

#ifndef __GHOST_LIBC_UNISTD__
#define __GHOST_LIBC_UNISTD__

#include "ghost/common.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/stat.h"

__BEGIN_C

// constants for use with <access>
#define	F_OK		0	// file existence
#define	X_OK		1	// execute or search permission
#define	W_OK		2	// write permission
#define	R_OK		3	// read permission

/**
 * POSIX wrapper for <g_read>
 */
ssize_t read(int fd, void* buf, size_t count);

/**
 * POSIX wrapper for <g_write>
 */
ssize_t write(int fd, const void* buf, size_t count);

/**
 * POSIX wrapper for <g_seek>
 */
off_t lseek(int fd, off_t offset, int whence);

/**
 * POSIX wrapper for <g_tell>
 */
long int tell(int filedes);

/**
 * POSIX wrapper for <g_close>
 */
int close(int filedes);

/**
 * POSIX wrapper for <g_sbrk>
 */
void* sbrk(intptr_t increment);

/**
 * TODO
 */
int isatty(int fd);

/**
 * TODO
 */
int access(const char* pathname, int mode);

/**
 * POSIX wrapper for <g_sleep>, but with seconds instead of milliseconds
 */
unsigned sleep(unsigned seconds);

__END_C

#endif
