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

#include "ghost.h"
#include "dirent.h"
#include "errno.h"
#include "malloc.h"

/**
 *
 */
DIR* opendir(const char* path) {

	g_fs_open_directory_status stat;
	g_fs_directory_iterator* iter = g_open_directory_s(path, &stat);

	if (stat == G_FS_OPEN_DIRECTORY_SUCCESSFUL) {
		DIR* dir = (DIR*) malloc(sizeof(DIR));
		dir->entbuf = (dirent*) malloc(sizeof(dirent));
		dir->iter = iter;
		return dir;

	} else if (stat == G_FS_OPEN_DIRECTORY_NOT_FOUND) {
		errno = ENOTDIR;

	} else if (stat == G_FS_OPEN_DIRECTORY_ERROR) {
		errno = EIO;
	}

	return NULL;
}
