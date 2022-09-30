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

#include "kernel/calls/syscall_tasking.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/wait.hpp"

#include "shared/logger/logger.hpp"

void syscallSleep(g_task* task, g_syscall_sleep* data)
{
	waitSleep(task, data->milliseconds);
}

void syscallYield(g_task* task)
{
	taskingYield();
}

void syscallExit(g_task* task, g_syscall_exit* data)
{
	taskingExit();
}

void syscallGetProcessId(g_task* task, g_syscall_get_pid* data)
{
	data->id = taskingGetCurrentTask()->process->id;
}

void syscallGetTaskId(g_task* task, g_syscall_get_tid* data)
{
	data->id = taskingGetCurrentTask()->id;
}

void syscallGetProcessIdForTaskId(g_task* task, g_syscall_get_pid_for_tid* data)
{
	g_task* theTask = taskingGetById(data->tid);
	if(theTask)
	{
		data->pid = theTask->process->id;
	}
	else
	{
		data->pid = G_PID_NONE;
	}
}

void syscallJoin(g_task* task, g_syscall_join* data)
{
	waitJoinTask(task, data->taskId);
}

void syscallSpawn(g_task* task, g_syscall_spawn* data)
{
	g_fd fd;
	g_fs_open_status open = filesystemOpen(data->path, G_FILE_FLAG_MODE_READ, task, &fd);
	if(open == G_FS_OPEN_SUCCESSFUL)
	{
		g_process* targetProcess;
		data->spawnStatus = taskingSpawn(task, fd, G_SECURITY_LEVEL_APPLICATION, &targetProcess, &data->validationDetails);
		if(data->spawnStatus == G_SPAWN_STATUS_SUCCESSFUL)
		{
			data->pid = targetProcess->id;
#warning TODO finish implementation (pass arguments etc.)
		}
	}
	else
	{
		data->spawnStatus = G_SPAWN_STATUS_IO_ERROR;
		logInfo("%! failed to find binary '%s'", "kernel", data->path);
	}
}

void syscallTaskGetTls(g_task* task, g_syscall_task_get_tls* data)
{
	data->userThreadObject = (void*) task->tlsCopy.userThreadObject;
}

void syscallProcessGetInfo(g_task* task, g_syscall_process_get_info* data)
{
	data->processInfo = task->process->userProcessInfo;
}

void syscallKill(g_task* task, g_syscall_kill* data)
{
	g_task* target = taskingGetById(data->pid);
	if(target)
	{
		target->process->main->status = G_THREAD_STATUS_DEAD;
		taskingYield();
	}
}

void syscallCreateThread(g_task* task, g_syscall_create_thread* data)
{
	mutexAcquire(&task->process->lock);

	g_task* thread = taskingCreateThread((g_virtual_address) data->initialEntry, task->process, task->process->main->securityLevel);
	if(thread)
	{
		thread->userEntry.function = data->userEntry;
		thread->userEntry.data = data->userData;
		data->threadId = thread->id;
		data->status = G_CREATE_THREAD_STATUS_SUCCESSFUL;
		taskingAssign(taskingGetLocal(), thread);
	}
	else
	{
		data->status = G_CREATE_THREAD_STATUS_FAILED;
	}

	mutexRelease(&task->process->lock);
}

void syscallGetThreadEntry(g_task* task, g_syscall_get_thread_entry* data)
{
	data->userData = task->userEntry.data;
	data->userEntry = task->userEntry.function;
}

void syscallFork(g_task* task, g_syscall_fork* data)
{
	logInfo("syscall not implemented: fork");
	for(;;)
		;
}

void syscallGetParentProcessId(g_task* task, g_syscall_get_parent_pid* data)
{
	logInfo("syscall not implemented: syscallGetParentProcessId");
	for(;;)
		;
}
