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

#include "ghost/syscall.h"
#include "ghost/filesystem.h"
#include "ghost/filesystem/callstructs.h"

// redirect
g_fs_stat_status g_fs_stat(const char* path, g_fs_stat_data* out)
{
	return g_fs_stat_l(path, out, true);
}

g_fs_stat_status g_fs_stat_l(const char* path, g_fs_stat_data* out, g_bool follow_symlinks)
{
	g_syscall_fs_stat data;
	data.path = path;
	data.out = out;
	data.follow_symlinks = follow_symlinks;

	g_syscall(G_SYSCALL_FS_STAT, (g_address) &data);

	return data.status;
}
