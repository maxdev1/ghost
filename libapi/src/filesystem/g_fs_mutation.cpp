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

#include "ghost/filesystem.h"
#include "ghost/filesystem/callstructs.h"
#include "ghost/syscall.h"

static g_fs_mutation_status __g_fs_mutate_path(uint32_t call, const char* path)
{
	if(!path || !*path)
		return G_FS_MUTATION_INVALID_ARGUMENT;

	g_syscall_fs_path_mutation data{path, G_FS_MUTATION_NOT_SUPPORTED};
	g_syscall(call, (g_address) &data);
	return data.status;
}

g_fs_mutation_status g_create_directory(const char* path)
{
	return __g_fs_mutate_path(G_SYSCALL_FS_CREATE_DIRECTORY, path);
}

g_fs_mutation_status g_unlink(const char* path) { return __g_fs_mutate_path(G_SYSCALL_FS_UNLINK, path); }

g_fs_mutation_status g_unlink_directory(const char* path)
{
	return __g_fs_mutate_path(G_SYSCALL_FS_UNLINK_DIRECTORY, path);
}

static g_fs_mutation_status __g_fs_mutate_descriptor(uint32_t call, g_fd fd, uint64_t length)
{
	if(fd == G_FD_NONE)
		return G_FS_MUTATION_INVALID_ARGUMENT;

	g_syscall_fs_fd_mutation data{fd, length, G_FS_MUTATION_NOT_SUPPORTED};
	g_syscall(call, (g_address) &data);
	return data.status;
}

g_fs_mutation_status g_truncate(g_fd fd, uint64_t length)
{
	return __g_fs_mutate_descriptor(G_SYSCALL_FS_TRUNCATE, fd, length);
}

g_fs_mutation_status g_sync(g_fd fd) { return __g_fs_mutate_descriptor(G_SYSCALL_FS_SYNC, fd, 0); }

g_fs_mutation_status g_rename(const char* source, const char* destination, g_bool replace)
{
	if(!source || !*source || !destination || !*destination)
		return G_FS_MUTATION_INVALID_ARGUMENT;

	g_syscall_fs_rename data{source, destination, replace, G_FS_MUTATION_NOT_SUPPORTED};
	g_syscall(G_SYSCALL_FS_RENAME, (g_address) &data);
	return data.status;
}

g_fs_mutation_status g_symlink(const char* target, const char* path)
{
	if(!target || !path || !*target || !*path)
		return G_FS_MUTATION_INVALID_ARGUMENT;
	g_syscall_fs_symlink data{target, path, G_FS_MUTATION_NOT_SUPPORTED};
	g_syscall(G_SYSCALL_FS_SYMLINK, (g_address) &data);
	return data.status;
}

int64_t g_readlink_s(const char* path, char* buffer, uint64_t capacity, g_fs_open_status* out_status)
{
	g_syscall_fs_readlink data{path, buffer, capacity, -1, G_FS_OPEN_INVALID_LINK};
	if(path && *path && buffer && capacity)
		g_syscall(G_SYSCALL_FS_READLINK, (g_address) &data);
	if(out_status) *out_status = data.status;
	return data.result;
}
