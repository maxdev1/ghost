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
#include <errno.h>


/**
 *
 */
int stat(const char* pathname, struct stat* buf)
{
	if(!pathname || !buf) { errno = EINVAL; return -1; }
	g_fs_stat_data data;
	g_fs_stat_status status = g_fs_stat(pathname, &data);
	if(status != G_FS_STAT_SUCCESS)
	{
		errno = __g_fs_stat_errno(status);
		return -1;
	}

	_stat_from_g_fs_stat(buf, &data);
	return 0;
}
