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
#include "kernel/logger/logger.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/memory_user.hpp"
#include "kernel/utils/string.hpp"

void syscallFsOpen(g_task* task, g_syscall_fs_open* data)
{
	char path[G_PATH_MAX];
	data->fd = G_FD_NONE;
	data->status = G_FS_OPEN_ERROR;
	if(!memoryCopyUserString(task, data->path, path, sizeof(path)))
		return;

	g_fd fd;
	data->status = filesystemOpen(path, data->flags, task, &fd);
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
	auto address = (g_address) data->buffer;
	data->result = -1;
	data->status = G_FS_READ_ERROR;

	if(data->length >= 0 && data->length <= INT32_MAX && address <= G_MEM_LOWER_HALF_END &&
	   uint64_t(data->length) <= G_MEM_LOWER_HALF_END - address + 1)
	{
		auto destination = memoryUserAcquire(task, address, data->length, true);
		auto bytes = memoryUserBuffer(destination);
		if(!bytes)
		{
			return;
		}

		int64_t read = -1;
		data->status = filesystemRead(task, data->fd, (uint8_t*) bytes, data->length, &read);
		data->result = data->status == G_FS_READ_SUCCESSFUL ? read : -1;
		if(read > 0)
			memoryUserWrite(destination, bytes, read);
	}
}

void syscallFsWrite(g_task* task, g_syscall_fs_write* data)
{
	auto address = (g_address) data->buffer;
	data->result = -1;
	data->status = G_FS_WRITE_ERROR;

	if(data->length >= 0 && data->length <= INT32_MAX && address <= G_MEM_LOWER_HALF_END &&
	   uint64_t(data->length) <= G_MEM_LOWER_HALF_END - address + 1)
	{
		auto source = memoryUserAcquire(task, address, data->length, false);
		auto bytes = memoryUserBuffer(source);
		if(!bytes || !memoryUserRead(source, bytes, data->length))
		{
			return;
		}

		int64_t written = -1;
		data->status = filesystemWrite(task, data->fd, (uint8_t*) bytes, data->length, &written);
		data->result = data->status == G_FS_WRITE_SUCCESSFUL ? written : -1;
	}
}

void syscallFsClose(g_task* task, g_syscall_fs_close* data)
{
	data->status = filesystemClose(task->process->id, data->fd, true);
}

void syscallFsLength(g_task* task, g_syscall_fs_length* data)
{
	uint64_t length = 0;
	data->status = filesystemGetLength(task, data->fd, &length);
	data->length = length;
}

void syscallFsCloneFd(g_task* task, g_syscall_fs_clonefd* data)
{
	data->status = filesystemProcessCloneDescriptor(data->source_pid, data->source_fd, data->target_pid,
													data->target_fd, &data->result);
}

void syscallFsTell(g_task* task, g_syscall_fs_tell* data)
{
	auto descriptor = filesystemProcessAcquireDescriptor(task->process->id, data->fd);
	if(!descriptor)
	{
		data->status = G_FS_TELL_INVALID_FD;
		data->result = G_FD_NONE;
		return;
	}

	mutexAcquire(&descriptor->operationLock);
	data->result = descriptor->offset;
	mutexRelease(&descriptor->operationLock);
	data->status = G_FS_TELL_SUCCESSFUL;
	filesystemProcessReleaseDescriptor(descriptor);
}

void syscallFsPipe(g_task* task, g_syscall_fs_pipe* data)
{
	data->write_fd = G_FD_NONE;
	data->read_fd = G_FD_NONE;
	g_fs_node* pipeNode;
	data->status = filesystemCreatePipe(data->blocking, &pipeNode);

	if(data->status != G_FS_PIPE_SUCCESSFUL)
	{
		logInfo("%! failed to create pipe for task %i with status %i", "filesystem", task->id, data->status);
		data->read_fd = G_FD_NONE;
		data->write_fd = G_FD_NONE;
		return;
	}

	g_file_flag_mode writeFlags = G_FILE_FLAG_MODE_WRITE;
	if(data->blocking)
		writeFlags |= G_FILE_FLAG_MODE_BLOCKING;
	g_fs_open_status writeOpen = filesystemOpenNodeFd(pipeNode, writeFlags, task->process->id, &data->write_fd);
	if(writeOpen != G_FS_OPEN_SUCCESSFUL)
	{
		logInfo("%! failed to open write end of pipe %i for task %i with status %i", "filesystem", pipeNode->id,
				task->id, writeOpen);
		data->status = G_FS_PIPE_ERROR;
		return;
	}

	// Keep readers blocking even for nonblocking pipes so dedicated consumer threads can sleep.
	g_file_flag_mode readFlags = (G_FILE_FLAG_MODE_READ | G_FILE_FLAG_MODE_BLOCKING);
	g_fs_open_status readOpen = filesystemOpenNodeFd(pipeNode, readFlags, task->process->id, &data->read_fd);
	if(readOpen != G_FS_OPEN_SUCCESSFUL)
	{
		if(filesystemClose(task->process->id, data->write_fd, true) != G_FS_CLOSE_SUCCESSFUL)
		{
			logInfo("%! failed to close write end of pipe %i for task %i after failing to open read end", "filesystem",
					pipeNode->id, task->id);
		}
		logInfo("%! failed to open read end of pipe %i for task %i with status %i", "filesystem", pipeNode->id,
				task->id, readOpen);
		data->status = G_FS_PIPE_ERROR;
		return;
	}

	data->status = G_FS_PIPE_SUCCESSFUL;
}

void syscallFsOpenDirectory(g_task* task, g_syscall_fs_open_directory* data)
{
	char path[G_PATH_MAX];
	data->status = G_FS_OPEN_DIRECTORY_ERROR;
	if(!memoryCopyUserString(task, data->path, path, sizeof(path)))
		return;

	auto iteratorOutput = memoryUserAcquire(task, (g_address) data->iterator, sizeof(g_fs_directory_iterator), true);
	g_fs_directory_iterator iterator;
	if(!iteratorOutput || !memoryUserRead(iteratorOutput, &iterator, sizeof(iterator)))
		return;

	g_fs_node* origin = nullptr;
	auto status = filesystemChooseOrigin(path, task, origin);
	if(status != G_FS_OPEN_SUCCESSFUL)
	{
		data->status = status == G_FS_OPEN_NOT_FOUND ? G_FS_OPEN_DIRECTORY_NOT_FOUND : G_FS_OPEN_DIRECTORY_ERROR;
		return;
	}

	auto findRes = filesystemFind(origin, path);
	if(findRes.status == G_FS_OPEN_SUCCESSFUL)
	{
		data->status = filesystemOpenDirectory(findRes.node);

		if(data->status == G_FS_OPEN_DIRECTORY_SUCCESSFUL)
		{
			iterator.node_id = findRes.node->id;
			iterator.position = 0;
			memoryUserWrite(iteratorOutput, &iterator, sizeof(iterator));
		}
	}
	else
	{
		data->status = static_cast<g_fs_open_directory_status>(findRes.status);
	}
}

void syscallFsReadDirectory(g_task* task, g_syscall_fs_read_directory* data)
{
	data->status = G_FS_READ_DIRECTORY_ERROR;
	auto iteratorOutput = memoryUserAcquire(task, (g_address) data->iterator, sizeof(g_fs_directory_iterator), true);
	g_fs_directory_iterator iterator;
	if(!iteratorOutput || !memoryUserRead(iteratorOutput, &iterator, sizeof(iterator)))
		return;

	g_fs_node* directory = filesystemGetNode(iterator.node_id);
	if(!directory)
	{
		data->status = G_FS_READ_DIRECTORY_ERROR;
		return;
	}

	if(directory->type == G_FS_NODE_TYPE_FILE)
	{
		data->status = G_FS_READ_DIRECTORY_ERROR;
		return;
	}

	g_fs_node* child = nullptr;
	data->status = filesystemReadDirectory(directory, iterator.position, &child);
	if(data->status == G_FS_READ_DIRECTORY_SUCCESSFUL)
	{
		iterator.entry_buffer.node_id = child->id;
		iterator.entry_buffer.type = child->type;

		char name[G_FILENAME_MAX];
		mutexAcquire(&child->lock);
		stringCopy(name, child->name);
		mutexRelease(&child->lock);

		if(!memoryCopyToUser(task, (g_address) iterator.entry_buffer.name, name, stringLength(name) + 1))
		{
			data->status = G_FS_READ_DIRECTORY_ERROR;
			return;
		}

		++iterator.position;
		memoryUserWrite(iteratorOutput, &iterator, sizeof(iterator));
	}
}

void syscallFsCloseDirectory(g_task* task, g_syscall_fs_close_directory* data)
{
	// TODO
}

void syscallFsRealPath(g_task* task, g_syscall_fs_real_path* data)
{
	char path[G_PATH_MAX];
	char result[G_PATH_MAX];
	data->status = G_FS_REAL_PATH_ERROR;
	if(!memoryCopyUserString(task, data->in, path, sizeof(path)))
		return;

	data->status = filesystemRealPath(task, path, result);
	if(data->status == G_FS_REAL_PATH_SUCCESS &&
	   !memoryCopyToUser(task, (g_address) data->out, result, stringLength(result) + 1))
		data->status = G_FS_REAL_PATH_ERROR;
}

void syscallFsStat(g_task* task, g_syscall_fs_stat* data)
{
	char path[G_PATH_MAX];
	g_fs_stat_data result{};
	data->status = G_FS_STAT_ERROR;
	if(!memoryCopyUserString(task, data->path, path, sizeof(path)))
		return;

	data->status = filesystemStat(task, path, &result, data->follow_symlinks);
	if(data->status == G_FS_STAT_SUCCESS && !memoryCopyToUser(task, (g_address) data->out, &result, sizeof(result)))
		data->status = G_FS_STAT_ERROR;
}

void syscallFsFstat(g_task* task, g_syscall_fs_fstat* data)
{
	g_fs_stat_data result{};
	data->status = filesystemFstat(task, data->fd, &result);
	if(data->status == G_FS_STAT_SUCCESS && !memoryCopyToUser(task, (g_address) data->out, &result, sizeof(result)))
		data->status = G_FS_STAT_ERROR;
}
