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
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/memory/heap.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/utils/hashmap.hpp"
#include "shared/logger/logger.hpp"
#include "shared/utils/string.hpp"

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
	data->millis = clockGetLocal()->time;
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
		}
		else
		{
			stringCopy(data->buffer, workingDirectory);
			data->result = G_GET_WORKING_DIRECTORY_SUCCESSFUL;
		}
	}
	else
	{
		stringCopy(data->buffer, "/");
		data->result = G_GET_WORKING_DIRECTORY_SUCCESSFUL;
	}
}

void syscallSetWorkingDirectory(g_task* task, g_syscall_fs_set_working_directory* data)
{
	auto findRes = filesystemFind(nullptr, data->path);
	if(findRes.status == G_FS_OPEN_SUCCESSFUL)
	{
		if(findRes.file->type == G_FS_NODE_TYPE_FOLDER || findRes.file->type == G_FS_NODE_TYPE_MOUNTPOINT || findRes.file->type == G_FS_NODE_TYPE_ROOT)
		{
			if(task->process->environment.workingDirectory)
			{
				heapFree(task->process->environment.workingDirectory);
			}

			int length = filesystemGetAbsolutePathLength(findRes.file);
			task->process->environment.workingDirectory = (char*) heapAllocate(length + 1);
			filesystemGetAbsolutePath(findRes.file, task->process->environment.workingDirectory);
			data->result = G_SET_WORKING_DIRECTORY_SUCCESSFUL;
		}
		else
		{
			data->result = G_SET_WORKING_DIRECTORY_NOT_A_FOLDER;
		}
	}
	else if(findRes.status == G_FS_OPEN_NOT_FOUND)
	{
		data->result = G_SET_WORKING_DIRECTORY_NOT_FOUND;
	}
	else
	{
		data->result = G_SET_WORKING_DIRECTORY_ERROR;
	}
}

void syscallKernQuery(g_task* task, g_syscall_kernquery* data)
{
	uint64_t start = clockGetLocal()->time;

	if(data->command == G_KERNQUERY_TASK_LIST)
	{
		g_kernquery_task_list_data* kdata = (g_kernquery_task_list_data*) data->buffer;

		uint32_t rem = kdata->id_buffer_size;
		kdata->filled_ids = 0;
		auto iter = hashmapIteratorStart(taskGlobalMap);
		while(rem-- && hashmapIteratorHasNext(&iter))
		{
			auto next = hashmapIteratorNext(&iter)->value;
			kdata->id_buffer[kdata->filled_ids++] = next->id;
		}
		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
		hashmapIteratorEnd(&iter);
	}
	else if(data->command == G_KERNQUERY_TASK_COUNT)
	{
		g_kernquery_task_count_data* kdata = (g_kernquery_task_count_data*) data->buffer;

		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
		kdata->count = hashmapSize(taskGlobalMap);
	}
	else if(data->command == G_KERNQUERY_TASK_GET_BY_ID)
	{
		g_kernquery_task_get_data* kdata = (g_kernquery_task_get_data*) data->buffer;

		g_task* ktask = taskingGetById(kdata->id);
		if(!ktask || ktask->status == G_THREAD_STATUS_DEAD)
		{
			data->status = G_KERNQUERY_STATUS_UNKNOWN_ID;
			kdata->id = -1;
		}
		else
		{
			data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
			kdata->found = true;
			kdata->id = ktask->id;
			kdata->parent = ktask->process->id;
			kdata->type = ktask->type;

			if(ktask->process->environment.executablePath)
				stringCopy(kdata->source_path, ktask->process->environment.executablePath);
			else
				kdata->source_path[0] = 0;

			const char* identifier = taskingDirectoryGetIdentifier(ktask->id);
			if(identifier)
				stringCopy(kdata->identifier, identifier);
			else
				kdata->identifier[0] = 0;

			kdata->memory_used = 0; // TODO
		}
	}
	else
	{
		data->status = G_KERNQUERY_STATUS_ERROR;
	}
}