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

#ifndef __GHOST_LIBC_DIRENT__
#define __GHOST_LIBC_DIRENT__

#include "ghost/common.h"
#include "stdio.h"

__BEGIN_C

typedef struct DIR DIR;

/**
 * Directory entry types
 */
#define DT_UNKNOWN		0
#define DT_REG			1
#define DT_DIR			2
#define DT_FIFO			3
#define DT_SOCK			4
#define DT_CHR			5
#define DT_BLK			6
#define DT_LNK			7

/**
 *
 */
typedef struct dirent dirent;

struct dirent {
	ino_t d_fileno;
	size_t d_reclen;
	size_t d_namlen;
	dev_t d_dev;
	unsigned char d_type;
	char d_name[G_FILENAME_MAX];
};

/**
 *
 */
int closedir(DIR* dir);

/**
 *
 */
DIR* opendir(const char* path);

/**
 *
 */
struct dirent* readdir(DIR* dir);

/**
 * Reentrant version of the readdir function.
 */
int readdir_r(DIR* dirp, struct dirent* entry, struct dirent** result);

/**
 *
 */
void rewinddir(DIR* dir);

__END_C

#endif
