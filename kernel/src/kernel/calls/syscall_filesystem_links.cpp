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

static g_fs_open_status syscallFsLinkString(g_task* task, const char* source, char* output)
{
	for(size_t i = 0; i < G_PATH_MAX; ++i)
	{
		auto address = (g_address) source;
		if(address > UINTPTR_MAX - i || !memoryCopyFromUser(task, output + i, address + i, 1))
			return G_FS_OPEN_ERROR;

		if(!output[i])
			return i ? G_FS_OPEN_SUCCESSFUL : G_FS_OPEN_INVALID_LINK;
	}
	return G_FS_OPEN_NAME_TOO_LONG;
}

void syscallFsSymlink(g_task* task, g_syscall_fs_symlink* data)
{
	data->status = G_FS_MUTATION_NAME_TOO_LONG;

	auto strings = static_cast<char*>(heapTryAllocate(2 * G_PATH_MAX));
	if(!strings)
	{
		data->status = G_FS_MUTATION_ERROR;
	}
	else
	{
		auto status = syscallFsLinkString(task, data->target, strings);
		if(status == G_FS_OPEN_SUCCESSFUL)
			status = syscallFsLinkString(task, data->path, strings + G_PATH_MAX);
		if(status == G_FS_OPEN_SUCCESSFUL)
			data->status = filesystemSymlink(task, strings, strings + G_PATH_MAX);
		else
			data->status = status == G_FS_OPEN_NAME_TOO_LONG  ? G_FS_MUTATION_NAME_TOO_LONG
						   : status == G_FS_OPEN_INVALID_LINK ? G_FS_MUTATION_INVALID_ARGUMENT
															  : G_FS_MUTATION_ERROR;

		heapFree(strings);
	}
}

void syscallFsReadlink(g_task* task, g_syscall_fs_readlink* data)
{
	data->result = -1;
	data->status = G_FS_OPEN_INVALID_LINK;

	auto strings = static_cast<char*>(heapTryAllocate(2 * G_PATH_MAX));
	if(!strings)
	{
		data->status = G_FS_OPEN_ERROR;
	}
	else
	{
		if(data->capacity)
			data->status = syscallFsLinkString(task, data->path, strings);

		if(data->status == G_FS_OPEN_SUCCESSFUL)
		{
			uint32_t length = 0;
			data->status = filesystemReadLinkPath(task, strings, strings + G_PATH_MAX, &length);
			if(data->status == G_FS_OPEN_SUCCESSFUL)
			{
				auto count = data->capacity < length ? data->capacity : length;
				if(memoryCopyToUser(task, (g_address) data->buffer, strings + G_PATH_MAX, count))
					data->result = count;
				else
					data->status = G_FS_OPEN_ERROR;
			}
		}
		heapFree(strings);
	}
}
