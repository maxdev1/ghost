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

#include "kernel/calls/syscall_general.hpp"
#include "kernel/tasking/wait.hpp"

#include "kernel/memory/heap.hpp"
#include "shared/logger/logger.hpp"
#include "shared/utils/string.hpp"

void syscallAtomicLock(g_task* task, g_syscall_atomic_lock* data)
{
	if(*data->atom_1 && (!data->atom_2 || *data->atom_2))
	{
		if(data->is_try)
		{
			data->was_set = false;
		} else
		{
			waitAtomicLock(task);
			taskingSchedule();
		}
	} else
	{
		*data->atom_1 = true;
		if(data->atom_2)
		{
			*data->atom_2 = true;
		}
		data->was_set = true;
	}
}

void syscallLog(g_task* task, g_syscall_log* data)
{
	logInfo("%! %i: %s", "log", task->id, data->message);
}

void syscallSetVideoLog(g_task* task, g_syscall_set_video_log* data)
{
	loggerEnableVideo(data->enabled);
}

void syscallTest(g_task* task, g_syscall_test* data)
{
	data->result = data->test;
}

void syscallReleaseCliArguments(g_task* task, g_syscall_cli_args_release* data)
{
	if(task->process->environment.arguments)
		stringCopy(data->buffer, task->process->environment.arguments);
	else
		data->buffer[0] = 0;
}

void syscallGetMilliseconds(g_task* task, g_syscall_millis* data)
{
	data->millis = taskingGetLocal()->time;
}

void syscallGetExecutablePath(g_task* task, g_syscall_fs_get_executable_path* data)
{
	if(task->process->environment.executablePath)
		stringCopy(data->buffer, task->process->environment.executablePath);
	else
		data->buffer[0] = 0;
}

void syscallGetWorkingDirectory(g_task* task, g_syscall_fs_get_working_directory* data)
{
	const char* workingDirectory = task->process->environment.workingDirectory;
	if(workingDirectory)
	{
		size_t length = stringLength(workingDirectory);
		if(length + 1 > data->maxlen)
		{
			data->result = G_GET_WORKING_DIRECTORY_SIZE_EXCEEDED;
		} else
		{
			stringCopy(data->buffer, workingDirectory);
			data->result = G_GET_WORKING_DIRECTORY_SUCCESSFUL;
		}
	} else
	{
		stringCopy(data->buffer, "/");
		data->result = G_GET_WORKING_DIRECTORY_SUCCESSFUL;
	}
}


void syscallSetWorkingDirectory(g_task* task, g_syscall_fs_set_working_directory* data)
{
	g_fs_node* child;
	g_fs_open_status openStatus = filesystemFind(0, data->path, &child);
	if(openStatus == G_FS_OPEN_SUCCESSFUL)
	{
		if(child->type == G_FS_NODE_TYPE_FOLDER || child->type == G_FS_NODE_TYPE_MOUNTPOINT || child->type == G_FS_NODE_TYPE_ROOT)
		{
			if(task->process->environment.workingDirectory)
			{
				heapFree(task->process->environment.workingDirectory);
			}
			
			int length = filesystemGetAbsolutePathLength(child);
			task->process->environment.workingDirectory = (char*) heapAllocate(length + 1);
			filesystemGetAbsolutePath(child, task->process->environment.workingDirectory);
			data->result = G_SET_WORKING_DIRECTORY_SUCCESSFUL;
		} else
		{
			data->result = G_SET_WORKING_DIRECTORY_NOT_A_FOLDER;
		}

	} else if(openStatus == G_FS_OPEN_NOT_FOUND)
	{
		data->result = G_SET_WORKING_DIRECTORY_NOT_FOUND;

	} else
	{
		data->result = G_SET_WORKING_DIRECTORY_ERROR;
	}
}

void syscallKernQuery(g_task* task, g_syscall_fs_set_working_directory* data)
{
	logInfo("syscall not implemented: syscallKernQuery");
	for(;;)
		;
}