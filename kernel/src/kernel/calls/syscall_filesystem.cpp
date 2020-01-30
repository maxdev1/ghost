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
	g_fd fd;
	data->status = filesystemOpen(data->path, data->flags, task, &fd);
	if(data->status == G_FS_OPEN_SUCCESSFUL)
	{
		data->fd = fd;
	} else
	{
		data->fd = G_FD_NONE;
	}
}

void syscallFsSeek(g_task* task, g_syscall_fs_seek* data)
{
	data->status = filesystemSeek(task, data->fd, data->mode, data->amount, &data->result);
}

void syscallFsRead(g_task* task, g_syscall_fs_read* data)
{
	data->status = filesystemRead(task, data->fd, data->buffer, data->length, &data->result);
	if(data->status != G_FS_READ_SUCCESSFUL)
	{
		data->result = G_FD_NONE;
	}
}

void syscallFsWrite(g_task* task, g_syscall_fs_write* data)
{
	data->status = filesystemWrite(task, data->fd, data->buffer, data->length, &data->result);
	if(data->status != G_FS_WRITE_SUCCESSFUL)
	{
		data->result = G_FD_NONE;
	}
}

void syscallFsClose(g_task* task, g_syscall_fs_close* data)
{
	data->status = filesystemClose(task->process->id, data->fd, true);
}

void syscallFsCloneFd(g_task* task, g_syscall_fs_clonefd* data)
{
	g_file_descriptor* descriptor = filesystemProcessGetDescriptor(data->source_pid, data->source_fd);
	if(!descriptor)
	{
		data->status = G_FS_CLONEFD_INVALID_SOURCE_FD;
		data->result = G_FD_NONE;
		return;
	}

	g_file_descriptor* clone = filesystemProcessCloneDescriptor(descriptor, data->target_pid, data->target_fd);
	if(!clone)
	{
		data->result = G_FD_NONE;
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
		data->result = G_FD_NONE;
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

void syscallFsPipe(g_task* task, g_syscall_fs_pipe* data)
{
	g_fs_node* pipeNode;
	data->status = filesystemCreatePipe(data->blocking, &pipeNode);

	if(data->status != G_FS_PIPE_SUCCESSFUL)
	{
		logInfo("%! failed to create pipe for task %i with status %i", "filesystem", task->id, data->status);
		data->read_fd = G_FD_NONE;
		data->write_fd = G_FD_NONE;
		return;
	}

	g_file_flag_mode writeFlags = (G_FILE_FLAG_MODE_WRITE | (data->blocking ? G_FILE_FLAG_MODE_BLOCKING : 0));
	g_fs_open_status writeOpen = filesystemOpen(pipeNode, writeFlags, task, &data->write_fd);
	if(writeOpen != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! failed to open write end of pipe %i for task %i with status %i", "filesystem", pipeNode->id, task->id, writeOpen);
		data->status = G_FS_PIPE_ERROR;
		return;
	}

	g_file_flag_mode readFlags = (G_FILE_FLAG_MODE_READ | (data->blocking ? G_FILE_FLAG_MODE_BLOCKING : 0));
	g_fs_open_status readOpen = filesystemOpen(pipeNode, readFlags, task, &data->read_fd);
	if(readOpen != G_FS_OPEN_SUCCESSFUL)
	{
		if(filesystemClose(task->process->id, writeOpen, true) != G_FS_CLOSE_SUCCESSFUL)
		{
			logInfo("%! failed to close write end of pipe %i for task %i after failing to open read end", "filesystem", pipeNode->id, task->id);
		}
		logInfo("%! failed to open read end of pipe %i for task %i with status %i", "filesystem", pipeNode->id, task->id, readOpen);
		data->status = G_FS_PIPE_ERROR;
		return;
	}

	data->status = G_FS_PIPE_SUCCESSFUL;
}
