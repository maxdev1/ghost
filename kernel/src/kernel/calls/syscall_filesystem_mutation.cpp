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

#include "kernel/calls/syscall_filesystem.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/memory_user.hpp"

static bool syscallFsCopyMutationPath(g_task* task, const char* source, char* path)
{
	for(size_t i = 0; i < G_PATH_MAX; ++i)
	{
		auto address = (g_address) source;
		if(address > UINTPTR_MAX - i || !memoryCopyFromUser(task, path + i, address + i, 1))
			return false;
		if(path[i])
			continue;

		while(i > 1 && path[i - 1] == '/')
			path[--i] = 0;
		return i != 0;
	}

	return false;
}

static void syscallFsMutatePath(g_task* task, g_syscall_fs_path_mutation* data,
								g_fs_mutation_status (*operation)(g_task*, const char*))
{
	data->status = G_FS_MUTATION_INVALID_ARGUMENT;

	auto path = static_cast<char*>(heapTryAllocate(G_PATH_MAX));
	if(!path)
		data->status = G_FS_MUTATION_ERROR;
	else
	{
		if(syscallFsCopyMutationPath(task, data->path, path))
			data->status = operation(task, path);
		heapFree(path);
	}
}

static g_fs_mutation_status syscallFsRemoveFile(g_task* task, const char* path)
{
	return filesystemRemove(task, path, false);
}

static g_fs_mutation_status syscallFsRemoveDirectory(g_task* task, const char* path)
{
	return filesystemRemove(task, path, true);
}

void syscallFsCreateDirectory(g_task* task, g_syscall_fs_path_mutation* data)
{
	syscallFsMutatePath(task, data, filesystemMakeDirectory);
}

void syscallFsUnlink(g_task* task, g_syscall_fs_path_mutation* data)
{
	syscallFsMutatePath(task, data, syscallFsRemoveFile);
}

void syscallFsUnlinkDirectory(g_task* task, g_syscall_fs_path_mutation* data)
{
	syscallFsMutatePath(task, data, syscallFsRemoveDirectory);
}

static void syscallFsMutateDescriptor(g_task* task, g_syscall_fs_fd_mutation* data, g_fs_delegate_request_type type)
{
	data->status = filesystemMutateDescriptor(task, data->fd, type, data->length);
}

void syscallFsTruncate(g_task* task, g_syscall_fs_fd_mutation* data)
{
	syscallFsMutateDescriptor(task, data, G_FS_DELEGATE_REQUEST_RESIZE);
}

void syscallFsSync(g_task* task, g_syscall_fs_fd_mutation* data)
{
	syscallFsMutateDescriptor(task, data, G_FS_DELEGATE_REQUEST_FLUSH);
}

void syscallFsRename(g_task* task, g_syscall_fs_rename* data)
{
	data->status = G_FS_MUTATION_INVALID_ARGUMENT;
	auto paths = static_cast<char*>(heapTryAllocate(2 * G_PATH_MAX));
	if(!paths)
	{
		data->status = G_FS_MUTATION_ERROR;
	}
	else
	{
		if(syscallFsCopyMutationPath(task, data->source, paths) &&
		   syscallFsCopyMutationPath(task, data->destination, paths + G_PATH_MAX))
			data->status = filesystemRename(task, paths, paths + G_PATH_MAX, data->replace);

		heapFree(paths);
	}
}
