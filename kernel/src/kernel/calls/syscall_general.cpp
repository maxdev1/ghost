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
