/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <errno.h>
#include <unistd.h>

int fsync(int fd)
{
	if(fd < 0)
	{
		errno = EBADF;
		return -1;
	}
	g_fs_mutation_status status = g_sync(fd);
	if(status == G_FS_MUTATION_NOT_FOUND)
	{
		errno = EBADF;
		return -1;
	}
	return __g_fs_mutation_result_set_errno(status);
}

int ftruncate(int fd, off_t length)
{
	if(fd < 0)
	{
		errno = EBADF;
		return -1;
	}
	if(length < 0)
	{
		errno = EINVAL;
		return -1;
	}
	g_fs_mutation_status status = g_truncate(fd, length);
	if(status == G_FS_MUTATION_NOT_FOUND || status == G_FS_MUTATION_READ_ONLY)
	{
		errno = status == G_FS_MUTATION_NOT_FOUND ? EBADF : EROFS;
		return -1;
	}
	return __g_fs_mutation_result_set_errno(status);
}
