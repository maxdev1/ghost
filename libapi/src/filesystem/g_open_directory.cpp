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

#include <stdlib.h>
#include <stdarg.h>

// redirect
g_fs_directory_iterator* g_open_directory(const char* path)
{
	return g_open_directory_s(path, nullptr);
}

/**
 *
 */
g_fs_directory_iterator* g_open_directory_s(const char* path, g_fs_open_directory_status* out_status)
{
	auto iterator = (g_fs_directory_iterator*) malloc(sizeof(g_fs_directory_iterator));
	iterator->entry_buffer.name = (char*) malloc(G_FILENAME_MAX);

	g_syscall_fs_open_directory data;
	data.path = (char*) path;
	data.iterator = iterator;
	g_syscall(G_SYSCALL_FS_OPEN_DIRECTORY, (g_address) &data);

	if(out_status)
		*out_status = data.status;

	if(data.status == G_FS_OPEN_DIRECTORY_SUCCESSFUL)
		return iterator;

	free(iterator);
	return nullptr;
}
