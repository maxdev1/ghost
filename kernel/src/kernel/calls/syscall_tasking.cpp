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
#include "kernel/filesystem/filesystem_process.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/atoms.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/utils/wait_queue.hpp"
#include "shared/logger/logger.hpp"
#include "shared/utils/string.hpp"

void syscallSleep(g_task* task, g_syscall_sleep* data)
{
	clockWaitForTime(task->id, clockGetLocal()->time + data->milliseconds);
	task->status = G_THREAD_STATUS_WAITING;
	taskingSchedule();
}

void syscallAtomicInitialize(g_task* task, g_syscall_atomic_initialize* data)
{
	data->atom = atomicCreate();
}

void syscallAtomicLock(g_task* task, g_syscall_atomic_lock* data)
{
	bool useTimeout = (data->timeout > 0);
	if(useTimeout)
		clockWaitForTime(task->id, clockGetLocal()->time + data->timeout);

	while(
		(!useTimeout || !(data->has_timeout = clockHasTimedOut(task->id))) &&
		!(data->was_set = atomicLock(task, data->atom, data->is_try, data->set_on_finish)))
	{
		if(data->is_try)
			break;

		atomicWaitForLock(data->atom, task->id);
		task->status = G_THREAD_STATUS_WAITING;
		taskingYield();
	}

	if(useTimeout)
		clockUnwaitForTime(task->id);
	atomicUnwaitForLock(data->atom, task->id);
}

void syscallAtomicUnlock(g_task* task, g_syscall_atomic_unlock* data)
{
	atomicUnlock(data->atom);
}

void syscallAtomicDestroy(g_task* task, g_syscall_atomic_destroy* data)
{
	atomicDestroy(data->atom);
}

void syscallYield(g_task* task)
{
	taskingSchedule();
}

void syscallExit(g_task* task, g_syscall_exit* data)
{
	taskingProcessKillAllTasks(task->process->id);
	taskingExit();
}

void syscallExitThread(g_task* task)
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
	taskingWaitForExit(data->taskId, task->id);
	task->status = G_THREAD_STATUS_WAITING;
	taskingYield();
}

void syscallSpawn(g_task* task, g_syscall_spawn* data)
{
	g_fd fd;
	g_fs_open_status open = filesystemOpen(data->path, G_FILE_FLAG_MODE_READ, task, &fd);
	if(open == G_FS_OPEN_SUCCESSFUL)
	{
		auto spawned = taskingSpawn(fd, G_SECURITY_LEVEL_APPLICATION);
		data->status = spawned.status;
		data->validationDetails = spawned.validation;

		filesystemClose(task->process->id, fd, true);

		if(data->status == G_SPAWN_STATUS_SUCCESSFUL)
		{
			data->pid = spawned.process->id;
			filesystemProcessCreateStdio(task->process->id, data->inStdio, spawned.process->id, data->outStdio);

			spawned.process->environment.executablePath = stringDuplicate(data->path);
			if(data->args)
				spawned.process->environment.arguments = stringDuplicate(data->args);
			if(data->workdir)
				spawned.process->environment.workingDirectory = stringDuplicate(data->workdir);

			spawned.process->main->status = G_THREAD_STATUS_RUNNING;
		}
	}
	else
	{
		data->status = G_SPAWN_STATUS_IO_ERROR;
		logInfo("%! failed to find binary '%s'", "kernel", data->path);
	}
}

void syscallTaskGetTls(g_task* task, g_syscall_task_get_tls* data)
{
	data->userThreadLocal = task->threadLocal.userThreadLocal;
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
		data->status = G_KILL_STATUS_SUCCESSFUL;
		target->process->main->status = G_THREAD_STATUS_DEAD;
		waitQueueWake(&target->waitersJoin);
		taskingYield();
	}
	else
	{
		data->status = G_KILL_STATUS_NOT_FOUND;
	}
}

void syscallCreateThread(g_task* task, g_syscall_create_thread* data)
{
	mutexAcquire(&task->process->lock);

	g_task* thread = taskingCreateTask((g_virtual_address) data->initialEntry, task->process, task->process->main->securityLevel);
	if(thread)
	{
		thread->userEntry.function = data->userEntry;
		thread->userEntry.data = data->userData;
		data->threadId = thread->id;
		data->status = G_CREATE_THREAD_STATUS_SUCCESSFUL;
		taskingAssignBalanced(thread);
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
}

void syscallGetParentProcessId(g_task* task, g_syscall_get_parent_pid* data)
{
	logInfo("syscall not implemented: syscallGetParentProcessId");
}
