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

#include "kernel/calls/syscall_filesystem.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/filesystem/filesystem_process.hpp"

#include "shared/logger/logger.hpp"

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

		if(filesystemFind(0, workingDirectoryPath, &relative) != G_FS_OPEN_SUCCESSFUL)
		{
			relative = 0;
		}
	}

	g_fs_node* file;
	g_fs_open_status status = filesystemFind(relative, data->path, &file);
	if(status == G_FS_OPEN_SUCCESSFUL)
	{
		if(data->flags & G_FILE_FLAG_MODE_TRUNCATE)
		{
			if(filesystemTruncate(file) != G_FS_OPEN_SUCCESSFUL)
			{
				logInfo("%! failed to truncate file %i", "filesystem", file->id);
				data->fd = -1;
				data->status = G_FS_OPEN_ERROR;
			}
		}
	} else if(status == G_FS_OPEN_NOT_FOUND)
	{
		if(data->flags & G_FILE_FLAG_MODE_CREATE)
		{
			if(filesystemCreateFile(relative, data->path, &file) != G_FS_OPEN_SUCCESSFUL)
			{
				logInfo("%! failed to create file '%s' in parent %i", "filesystem", data->path, relative->id);
				data->fd = -1;
				data->status = G_FS_OPEN_ERROR;
				return;
			}
		} else
		{
			data->fd = -1;
			data->status = G_FS_OPEN_NOT_FOUND;
			return;
		}
	} else
	{
		data->fd = -1;
		data->status = status;
		return;
	}

	g_file_descriptor* descriptor = filesystemProcessCreateDescriptor(task->process->id, file->id, data->flags);
	data->fd = descriptor->id;
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

	uint64_t length;
	if(filesystemGetLength(node, &length) != G_FS_LENGTH_SUCCESSFUL)
	{
		logInfo("%! failed to seek in file %i, could not get length", "filesystem", node->id);
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

void syscallFsWrite(g_task* task, g_syscall_fs_write* data)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(task->process->id, data->fd);
	if(!descriptor)
	{
		data->status = G_FS_WRITE_INVALID_FD;
		data->result = -1;
		return;
	}

	g_fs_node* node = filesystemGetNode(descriptor->nodeId);
	if(!node)
	{
		data->status = G_FS_WRITE_INVALID_FD;
		data->result = -1;
		return;
	}

	uint64_t startOffset = descriptor->offset;
	if(descriptor->openFlags & G_FILE_FLAG_MODE_APPEND)
	{
		if(filesystemGetLength(node, &startOffset) != G_FS_LENGTH_SUCCESSFUL)
		{
			logInfo("%! failed to append to file %i, could not get length", "filesystem", node->id);
			data->status = G_FS_WRITE_ERROR;
			data->result = -1;
			return;
		}
	}

	int64_t wrote;
	data->status = filesystemWrite(node, data->buffer, startOffset, data->length, &wrote);
	if(wrote > 0)
	{
		descriptor->offset = startOffset + wrote;
	}
	data->result = wrote;
}

void syscallFsCloneFd(g_task* task, g_syscall_fs_clonefd* data)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(data->source_pid, data->source_fd);
	if(!descriptor)
	{
		data->status = G_FS_CLONEFD_INVALID_SOURCE_FD;
		data->result = -1;
		return;
	}

	g_file_descriptor* clone = filesystemProcessCloneDescriptor(descriptor, data->target_pid, data->target_fd);
	if(!clone)
	{
		data->result = -1;
		data->status = G_FS_CLONEFD_ERROR;
		return;
	}

	data->result = clone->id;
	data->status = G_FS_CLONEFD_SUCCESSFUL;
}

void syscallFsTell(g_task* task, g_syscall_fs_tell* data)
{

	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(task->process->id, data->fd);
	if(!descriptor)
	{
		data->status = G_FS_TELL_INVALID_FD;
		data->result = -1;
		return;
	}

	data->result = descriptor->offset;
	data->status = G_FS_TELL_SUCCESSFUL;
}

void syscallFsStat(g_task* task, g_syscall_fs_stat* data)
{
	logInfo("%! stat not implemented", "syscall");
}

void syscallFsFstat(g_task* task, g_syscall_fs_fstat* data)
{
	logInfo("%! fstat not implemented", "syscall");
}
