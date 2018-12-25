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
#include "string.h"

/**
 *
 */
struct dirent* readdir(DIR* dir) {

	g_fs_read_directory_status stat;
	g_fs_directory_entry* entry = g_read_directory_s(dir->iter, &stat);

	if (stat == G_FS_READ_DIRECTORY_SUCCESSFUL) {

		dirent* ent = dir->entbuf;
		ent->d_fileno = entry->node_id;
		dir->entbuf->d_dev = -1; // TODO
		dir->entbuf->d_namlen = strlen(entry->name);
		dir->entbuf->d_reclen = -1; // TODO
		dir->entbuf->d_type = -1; // TODO
		strcpy(ent->d_name, entry->name);
		return ent;

	} else if (stat == G_FS_READ_DIRECTORY_EOD) {
		return NULL;

	} else if (stat == G_FS_READ_DIRECTORY_ERROR) {
		errno = EIO;
	}

	return NULL;
}

/**
 *
 */
int readdir_r(DIR* dirp, struct dirent* entry, struct dirent** result) {

	g_atomic_lock(&(dirp->lock));

	errno = 0;

	struct dirent* entryFound = readdir(dirp);
	if (entryFound) {
		memcpy(entry, dirp->entbuf, sizeof(struct dirent));
	} else {
		*result = entry;
		*result = NULL;
	}

	int err = errno;
	dirp->lock = 0;
	return err ? err : 0;
}
