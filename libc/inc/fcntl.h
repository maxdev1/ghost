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

#ifndef __GHOST_LIBC_FCNTL__
#define __GHOST_LIBC_FCNTL__

#include <ghost/filesystem.h>
#include <ghost/stdint.h>

__BEGIN_C

// mode type
typedef uint32_t mode_t;

// POSIX open flags
#define O_READ				G_FILE_FLAG_MODE_READ
#define O_WRITE				G_FILE_FLAG_MODE_WRITE
#define O_APPEND			G_FILE_FLAG_MODE_APPEND
#define O_CREAT 			G_FILE_FLAG_MODE_CREATE
#define O_TRUNC				G_FILE_FLAG_MODE_TRUNCATE
#define O_EXCL				G_FILE_FLAG_MODE_EXCLUSIVE

#define	O_RDONLY			G_FILE_FLAG_MODE_READ
#define O_WRONLY 			G_FILE_FLAG_MODE_WRITE
#define O_RDWR				(G_FILE_FLAG_MODE_READ | G_FILE_FLAG_MODE_WRITE)

// commands & flags for fcntl()
// TODO
#define	F_DUPFD				0//G_FILE_CONTROL_DUPLICATE_DESCRIPTOR
#define F_GETFD				0//G_FILE_CONTROL_GET_DESCRIPTOR_FLAGS
#define F_SETFD				0//G_FILE_CONTROL_SET_DESCRIPTOR_FLAGS
#define F_SETFL				0//G_FILE_CONTROL_SET_STATUS_FLAGS
#define F_GETLK				0//G_FILE_CONTROL_GET_REC_LOCK_INFO
#define F_SETLK				0//G_FILE_CONTROL_SET_REC_LOCK_INFO
#define F_SETLKW			0//G_FILE_CONTROL_SET_REC_LOCK_INFO_WAIT
// #define F_GETOWN
// #define F_SETOWN

#define FD_CLOEXEC			0//G_FILE_CONTROL_FLAG_CLOSE_ON_EXEC

// POSIX
int open(const char* pathname, int flags, ...);

__END_C

#endif
