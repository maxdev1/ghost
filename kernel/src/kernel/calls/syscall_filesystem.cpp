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

#include "kernel/calls/syscall_fs.hpp"
#include "shared/logger/logger.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/filesystem/filesystem_process.hpp"

void syscallFsOpen(g_task* task, g_syscall_fs_open* data)
{
	g_fs_node* relative = 0;

	if(data->path[0] != '/')
	{
		const char* workingDirectoryPath = task->process->environment.workingDirectory;
		if(workingDirectoryPath == 0)
		{
			workingDirectoryPath = "/";
		}
		relative = filesystemFind(0, workingDirectoryPath);
	}

	g_fs_node* file = filesystemFind(relative, data->path);
	if(!file)
	{
		data->status = G_FS_OPEN_NOT_FOUND;
		return;
	}

	g_fd fd = filesystemProcessCreateDescriptor(task->process->id, file, data->flags);
	data->fd = fd;
	data->status = G_FS_OPEN_SUCCESSFUL;
}

void syscallFsSeek(g_task* task, g_syscall_fs_seek* data)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(task->process->id, data->fd);
	if(!descriptor)
	{
		data->status = G_FS_SEEK_INVALID_FD;
		return;
	}

	g_fs_node* node = filesystemGetNode(descriptor->nodeId);
	if(!node)
	{
		data->status = G_FS_SEEK_INVALID_FD;
		return;
	}

	int64_t length;
	if(filesystemGetLength(node, &length) != G_FS_LENGTH_SUCCESSFUL)
	{
		data->status = G_FS_SEEK_ERROR;
		return;
	}

	// add amount to offset
	if(data->mode == G_FS_SEEK_CUR)
	{
		descriptor->offset += data->amount;
	} else if(data->mode == G_FS_SEEK_SET)
	{
		descriptor->offset = data->amount;
	} else if(data->mode == G_FS_SEEK_END)
	{
		descriptor->offset = length - data->amount;
	}

	// validate offset
	if(descriptor->offset > length)
	{
		descriptor->offset = length;
	}
	if(descriptor->offset < 0)
	{
		descriptor->offset = 0;
	}

	data->result = descriptor->offset;
	data->status = G_FS_SEEK_SUCCESSFUL;
}

void syscallFsRead(g_task* task, g_syscall_fs_read* data)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(task->process->id, data->fd);
	if(!descriptor)
	{
		data->status = G_FS_READ_INVALID_FD;
		data->result = -1;
		return;
	}

	g_fs_node* node = filesystemGetNode(descriptor->nodeId);
	if(!node)
	{
		data->status = G_FS_READ_INVALID_FD;
		data->result = -1;
		return;
	}

	int64_t read;
	data->status = filesystemRead(node, data->buffer, descriptor->offset, data->length, &read);
	if(read > 0)
	{
		descriptor->offset += read;
	}
	data->result = read;
}

void syscallFsClose(g_task* task, g_syscall_fs_close* data)
{
	filesystemProcessRemoveDescriptor(task->process->id, data->fd);
}

