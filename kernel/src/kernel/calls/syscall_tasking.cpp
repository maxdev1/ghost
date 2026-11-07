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
#include "kernel/logger/logger.hpp"
#include "kernel/memory/constants.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/memory_user.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/scheduler/scheduler.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/tasking/user_mutex.hpp"
#include "kernel/utils/string.hpp"
#include "kernel/utils/wait_queue.hpp"

void syscallSleep(g_task* task, g_syscall_sleep* data)
{
	taskingWait(task, __func__,
				[task, data]() { clockWaitForTime(task->id, clockGetMilliseconds() + data->milliseconds); });
}

void syscallYield(g_task* task, g_syscall_yield* data)
{
	if(data->target != G_TID_NONE)
		schedulerPrefer(data->target);
	taskingYield();
}

void syscallExit(g_task* task, g_syscall_exit* data)
{
	waitQueueWake(&task->process->main->waitersJoin);
	taskingProcessKillAllTasks(task->process->id);
	taskingExit();
}

void syscallExitTask(g_task* task)
{
	waitQueueWake(&task->waitersJoin);
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
	auto theTask = taskingAcquireTask(data->tid);
	if(theTask)
	{
		data->pid = theTask->process->id;
	}
	else
	{
		data->pid = G_PID_NONE;
	}
	taskingReleaseTask(theTask);
}

void syscallJoin(g_task* task, g_syscall_join* data)
{
	while(taskingWaitIf([&]() { return taskingWaitForExit(data->taskId, task); }))
	{
	}
}

void syscallSpawn(g_task* task, g_syscall_spawn* data)
{
	char path[G_PATH_MAX];
	char arguments[G_CLIARGS_BUFFER_LENGTH];
	char workdir[G_PATH_MAX];

	data->status = G_SPAWN_STATUS_TASKING_ERROR;
	data->pid = G_PID_NONE;
	if(!memoryCopyUserString(task, data->path, path, sizeof(path)) ||
	   (data->args && !memoryCopyUserString(task, data->args, arguments, sizeof(arguments))) ||
	   (data->workdir && !memoryCopyUserString(task, data->workdir, workdir, sizeof(workdir))))
		return;

	auto mode = data->mode;
	if(mode == G_LAUNCH_MODE_INHERIT)
		mode = task->process->environment.launchMode;
	if(mode > G_LAUNCH_MODE_TERMINAL)
	{
		data->status = G_SPAWN_STATUS_TASKING_ERROR;
		return;
	}

	const g_security_level securityLevel = data->securityLevel;
	if(securityLevel > G_SECURITY_LEVEL_APPLICATION || securityLevel < task->securityLevel)
	{
		data->status = G_SPAWN_STATUS_TASKING_ERROR;
		logInfo("%! task %i doesn't have privilege to spawn '%s' as security level %i", "kernel", task->id, path,
				securityLevel);
		return;
	}

	g_fd fd;
	g_fs_open_status open = filesystemOpen(path, G_FILE_FLAG_MODE_READ, task, &fd);
	if(open == G_FS_OPEN_SUCCESSFUL)
	{
		auto target = taskingSpawn(fd, securityLevel);
		data->status = target.status;
		data->validationDetails = target.validation;

		filesystemClose(task->process->id, fd, true);

		if(data->status == G_SPAWN_STATUS_SUCCESSFUL)
		{
			data->pid = target.process->id;
			filesystemProcessCreateStdio(task->process->id, data->inStdio, target.process->id, data->outStdio);

			target.process->environment.launchMode = mode;
			target.process->environment.executablePath = stringDuplicate(path);
			if(data->args)
				target.process->environment.arguments = stringDuplicate(arguments);
			if(data->workdir)
				target.process->environment.workingDirectory = stringDuplicate(workdir);

			taskingReleaseSpawn(target.process);
		}
	}
	else
	{
		data->status = G_SPAWN_STATUS_IO_ERROR;
		logInfo("%! failed to find binary '%s'", "kernel", data->path);
	}
}

void syscallGetLaunchMode(g_task* task, g_syscall_get_launch_mode* data)
{
	data->mode = task->process->environment.launchMode;
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
	auto target = taskingAcquireTask(data->pid);
	if(target && target->securityLevel < task->securityLevel)
	{
		data->status = G_KILL_STATUS_FAILED;
	}
	else if(target)
	{
		data->status = G_KILL_STATUS_SUCCESSFUL;
		taskingTerminateProcess(target->process);
	}
	else
	{
		data->status = G_KILL_STATUS_NOT_FOUND;
	}
	taskingReleaseTask(target);
}

void syscallCreateTask(g_task* task, g_syscall_create_task* data)
{
	const uint8_t affinity = data->coreAffinity;
	const g_virtual_address initialEntry = (g_virtual_address) data->initialEntry;
	void* const userEntry = data->userEntry;
	void* const userData = data->userData;
	data->threadId = G_TID_NONE;
	data->status = G_CREATE_TASK_STATUS_FAILED;

	if(task->securityLevel != G_SECURITY_LEVEL_KERNEL && (!initialEntry || initialEntry > G_MEM_LOWER_HALF_END))
		return;

	if(affinity != G_TASK_CORE_AFFINITY_NONE && affinity >= processorGetNumberOfProcessors())
		return;

	g_task* newTask = taskingCreateTask(initialEntry, task->process, task->process->main->securityLevel);
	if(newTask)
	{
		newTask->userEntry.function = userEntry;
		newTask->userEntry.data = userData;
		data->threadId = newTask->id;
		data->status = G_CREATE_TASK_STATUS_SUCCESSFUL;

		if(affinity == G_TASK_CORE_AFFINITY_NONE)
		{
			taskingAssignBalanced(newTask);
		}
		else
		{
			taskingAssignOnCore(affinity, newTask);
		}
	}
	else
	{
		data->status = G_CREATE_TASK_STATUS_FAILED;
	}
}

void syscallGetTaskEntry(g_task* task, g_syscall_get_task_entry* data)
{
	data->userData = task->userEntry.data;
	data->userEntry = task->userEntry.function;
}

void syscallFork(g_task* task, g_syscall_fork* data) { logInfo("syscall not implemented: fork"); }

void syscallGetParentProcessId(g_task* task, g_syscall_get_parent_pid* data)
{
	logInfo("syscall not implemented: syscallGetParentProcessId");
}

void syscallGetTime(g_task*, g_syscall_time* data)
{
	data->unixSeconds = clockGetWallTime();
}

void syscallGetMilliseconds(g_task* task, g_syscall_millis* data)
{
	data->millis = clockGetMilliseconds();
}

void syscallGetNanoseconds(g_task* task, g_syscall_nanos* data)
{
	data->nanos = clockGetNanoseconds(processorGetCurrentId());
}

void syscallGetExecutablePath(g_task* task, g_syscall_get_executable_path* data)
{
	auto path = task->process->environment.executablePath;
	if(!path)
		path = "";
	memoryCopyToUser(task, (g_address) data->buffer, path, stringLength(path) + 1);
}

void syscallGetWorkingDirectory(g_task* task, g_syscall_get_working_directory* data)
{
	mutexAcquire(&task->process->lock);
	auto current = task->process->environment.workingDirectory;
	auto snapshot = stringDuplicate(current ? current : "/");
	mutexRelease(&task->process->lock);
	if(stringLength(snapshot) + 1 > data->maxlen)
		data->result = G_GET_WORKING_DIRECTORY_SIZE_EXCEEDED;
	else
	{
		data->result = memoryCopyToUser(task, (g_address) data->buffer, snapshot, stringLength(snapshot) + 1)
							   ? G_GET_WORKING_DIRECTORY_SUCCESSFUL
							   : G_GET_WORKING_DIRECTORY_SIZE_EXCEEDED;
	}
	heapFree(snapshot);
}

void syscallSetWorkingDirectory(g_task* task, g_syscall_set_working_directory* data)
{
	char path[G_PATH_MAX];
	data->result = G_SET_WORKING_DIRECTORY_ERROR;
	if(!memoryCopyUserString(task, data->path, path, sizeof(path)))
		return;

	auto findRes = filesystemFind(nullptr, path);
	if(findRes.status == G_FS_OPEN_SUCCESSFUL)
	{
		if(findRes.node->type == G_FS_NODE_TYPE_FOLDER || findRes.node->type == G_FS_NODE_TYPE_MOUNTPOINT ||
		   findRes.node->type == G_FS_NODE_TYPE_ROOT)
		{
			int length = filesystemGetAbsolutePathLength(findRes.node);
			if(length >= G_PATH_MAX)
			{
				data->result = G_SET_WORKING_DIRECTORY_ERROR;
				return;
			}
			auto replacement = static_cast<char*>(heapTryAllocate(length + 1));
			if(!replacement || filesystemGetAbsolutePath(findRes.node, replacement, length + 1) < 0)
			{
				if(replacement)
					heapFree(replacement);
				data->result = G_SET_WORKING_DIRECTORY_ERROR;
				return;
			}

			mutexAcquire(&task->process->lock);
			auto previous = task->process->environment.workingDirectory;
			task->process->environment.workingDirectory = replacement;
			mutexRelease(&task->process->lock);

			if(previous)
				heapFree(previous);
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

void syscallReleaseCliArguments(g_task* task, g_syscall_cli_args_release* data)
{
	auto arguments = task->process->environment.arguments;
	if(!arguments)
		arguments = "";
	memoryCopyToUser(task, (g_address) data->buffer, arguments, stringLength(arguments) + 1);
}

void syscallTaskRegisterName(g_task* task, g_syscall_task_register_name* data)
{
	char name[G_FILENAME_MAX];
	data->successful = memoryCopyUserString(task, data->name, name, sizeof(name)) &&
					   taskingDirectoryRegister(name, task->id, task->securityLevel);
}

void syscallTaskAwaitByName(g_task* task, g_syscall_task_await_by_name* data)
{
	char name[G_FILENAME_MAX];
	data->task = G_TID_NONE;
	if(!memoryCopyUserString(task, data->name, name, sizeof(name)))
		return;

	g_tid target;
	while((target = taskingDirectoryGet(name)) == G_TID_NONE)
	{
		if(taskingWaitIf([&]() { return taskingDirectoryWaitForRegister(name, task); }))
			taskingDirectoryUnwaitForRegister(name, task->id);
	}
	data->task = target;
}

void syscallGetTaskByName(g_task* task, g_syscall_task_get_by_name* data)
{
	char name[G_FILENAME_MAX];
	data->resultTaskId =
			memoryCopyUserString(task, data->name, name, sizeof(name)) ? taskingDirectoryGet(name) : G_TID_NONE;
}

void syscallDump() { schedulerDump(); }
