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
#include "kernel/system/interrupts/requests.hpp"
#include "shared/logger/logger.hpp"
#include "shared/utils/string.hpp"

void syscallFsOpen(g_task* task, g_syscall_fs_open* data)
{
	g_fd fd;
	data->status = filesystemOpen(data->path, data->flags, task, &fd);
	if(data->status == G_FS_OPEN_SUCCESSFUL)
	{
		data->fd = fd;
	}
	else
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

void syscallFsLength(g_task* task, g_syscall_fs_length* data)
{
	uint64_t length;
	data->status = filesystemGetLength(task, data->fd, &length);
	data->length = length;
}

void syscallFsCloneFd(g_task* task, g_syscall_fs_clonefd* data)
{
	data->status = filesystemProcessCloneDescriptor(data->source_pid, data->source_fd, data->target_pid, data->target_fd, &data->result);
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
	g_fs_open_status writeOpen = filesystemOpenNodeFd(pipeNode, writeFlags, task->process->id, &data->write_fd);
	if(writeOpen != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! failed to open write end of pipe %i for task %i with status %i", "filesystem", pipeNode->id, task->id, writeOpen);
		data->status = G_FS_PIPE_ERROR;
		return;
	}

	g_file_flag_mode readFlags = (G_FILE_FLAG_MODE_READ | (data->blocking ? G_FILE_FLAG_MODE_BLOCKING : 0));
	g_fs_open_status readOpen = filesystemOpenNodeFd(pipeNode, readFlags, task->process->id, &data->read_fd);
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

void syscallOpenIrqDevice(g_task* task, g_syscall_open_irq_device* data)
{
	if(task->securityLevel <= G_SECURITY_LEVEL_DRIVER)
	{
		g_irq_device* irqDevice = requestsGetIrqDevice(data->irq);
		if(!irqDevice)
		{
			data->status = G_OPEN_IRQ_DEVICE_STATUS_ERROR;
			logInfo("%! task %i: failed to retrieve device for irq %i", "irq", task->id, data->irq);
			return;
		}

		irqDevice->task = task->id;

		g_fd fd;
		g_file_flag_mode readFlags = (G_FILE_FLAG_MODE_READ | G_FILE_FLAG_MODE_BLOCKING);
		if(filesystemOpenNodeFd(irqDevice->node, readFlags, task->process->id, &fd) == G_FS_OPEN_SUCCESSFUL)
		{
			data->status = G_OPEN_IRQ_DEVICE_STATUS_SUCCESSFUL;
			data->fd = fd;
			logDebug("%! task %i: opened device for IRQ %i", "irq", task->id, data->irq);
		}
		else
		{
			data->status = G_OPEN_IRQ_DEVICE_STATUS_ERROR;
			logInfo("%! task %i: failed to open device for IRQ %i", "irq", task->id, data->irq);
		}
	}
	else
	{
		data->status = G_OPEN_IRQ_DEVICE_STATUS_NOT_PERMITTED;
		logInfo("%! task %i: not permitted to open device for irq %i", "irq", task->id, data->irq);
	}
}

void syscallFsOpenDirectory(g_task* task, g_syscall_fs_open_directory* data)
{
	auto findRes = filesystemFind(nullptr, data->path);
	if(findRes.status == G_FS_OPEN_SUCCESSFUL)
	{
		data->status = filesystemOpenDirectory(findRes.file);

		if(data->status == G_FS_OPEN_DIRECTORY_SUCCESSFUL)
		{
			data->iterator->node_id = findRes.file->id;
			data->iterator->position = 0;
		}
	}
	else
	{
		data->status = G_FS_OPEN_DIRECTORY_NOT_FOUND;
	}
}

void syscallFsReadDirectory(g_task* task, g_syscall_fs_read_directory* data)
{
	g_fs_node* directory = filesystemGetNode(data->iterator->node_id);
	if(!directory)
	{
		data->status = G_FS_READ_DIRECTORY_ERROR;
		return;
	}

	g_fs_node* child;
	data->status = filesystemReadDirectory(directory, data->iterator->position, &child);
	if(data->status == G_FS_READ_DIRECTORY_SUCCESSFUL)
	{
		data->iterator->entry_buffer.node_id = child->id;
		data->iterator->entry_buffer.type = child->type;
		stringCopy(data->iterator->entry_buffer.name, child->name);
		++data->iterator->position;
	}
}

void syscallFsCloseDirectory(g_task* task, g_syscall_fs_close_directory* data)
{
	// TODO: Is this needed?
}
