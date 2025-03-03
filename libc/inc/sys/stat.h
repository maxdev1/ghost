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

#ifndef __GHOST_LIBC_SYS_STAT__
#define __GHOST_LIBC_SYS_STAT__

#include "ghost/stdint.h"
#include "ghost/common.h"
#include "sys/types.h"
#include "time.h"

__BEGIN_C

typedef uint32_t mode_t;

// POSIX stat flags
// TODO kernel must understand these
#define S_IXOTH 01
#define S_IWOTH 02
#define S_IROTH 03
#define S_IRWXO 07
#define S_IXGRP 010
#define S_IWGRP 020
#define S_IRGRP 040
#define S_IRWXG 070
#define S_IXUSR 0100
#define S_IWUSR 0200
#define S_IRUSR 0400
#define S_IRWXU 0700

#define S_IFMT  0170000  // Bit mask for the file type
#define S_IFREG 0100000  // Regular file
#define S_IFDIR 0040000  // Directory
#define S_IFLNK 0120000  // Symbolic link
#define S_IFCHR 0020000  // Character device
#define S_IFBLK 0060000  // Block device
#define S_IFIFO 0010000  // FIFO (named pipe)
#define S_IFSOCK 0140000 // Socket

#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#define S_ISLNK(mode)  (((mode) & S_IFMT) == S_IFLNK)
#define S_ISCHR(mode)  (((mode) & S_IFMT) == S_IFCHR)
#define S_ISBLK(mode)  (((mode) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)

struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	off_t st_size;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
};

/**
 * TODO
 */
int stat(const char *pathname, struct stat *buf);
int fstat(int fd, struct stat* buf);
int lstat(const char *pathname, struct stat *buf);
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
mode_t umask(mode_t mask);
int mkdir(const char *path, mode_t mode);

__END_C

#endif
