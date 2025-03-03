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

#include "sys/stat.h"
#include "ghost/filesystem.h"
#include "internal.h"

void _stat_from_g_fs_stat(struct stat* to, g_fs_stat_data* from)
{
	to->st_dev = from->device;
	to->st_atime = from->time_last_access;
	to->st_mtime = from->time_last_modification;
	to->st_ctime = from->time_creation;
	to->st_ino = from->virtual_id;
	to->st_size = from->size;

	mode_t mode = 0;
	switch(from->type)
	{
		case G_FS_NODE_TYPE_FILE:
			mode |= S_IFREG;
			break;
		case G_FS_NODE_TYPE_FOLDER:
		case G_FS_NODE_TYPE_MOUNTPOINT:
		case G_FS_NODE_TYPE_ROOT:
			mode |= S_IFDIR;
			break;
		case G_FS_NODE_TYPE_PIPE:
			mode |= S_IFIFO;
			break;
		default:
			break;
	}
	to->st_mode = mode;

	// TODO
	to->st_blksize = -1;
	to->st_blocks = -1;
	to->st_nlink = -1;
	to->st_uid = -1;
	to->st_gid = -1;
}
