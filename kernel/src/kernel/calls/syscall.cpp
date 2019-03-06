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

#include "ghost/calls/calls.h"
#include "kernel/calls/syscall.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/kernel.hpp"

#include "kernel/calls/syscall_general.hpp"
#include "kernel/calls/syscall_fs.hpp"

static g_syscall_registration* syscallRegistrations = 0;

void syscallHandle(g_task* task)
{
	uint32_t callId = task->state->eax;
	void* syscallData = (void*) task->state->ebx;

	if(callId > G_SYSCALL_MAX)
	{
		logInfo("%! task %i tried to use out-of-range syscall %i", "syscall", task->id, callId);
		return;
	}

	g_syscall_registration* reg = &syscallRegistrations[callId];
	if(reg->handler == 0)
	{
		logInfo("%! task %i tried to use unknown syscall %i", "syscall", task->id, callId);
		return;
	}

	if(reg->threaded)
		syscallRunThreaded(reg->handler, task, syscallData);
	else
		reg->handler(task, syscallData);
}

void syscallRunThreaded(g_syscall_handler handler, g_task* caller, void* syscallData)
{
	mutexAcquire(&taskingGetLocal()->lock);

	g_task* proc = caller->syscall.processingTask;
	if(proc == 0)
	{
		proc = taskingCreateThread(0, caller->process, G_SECURITY_LEVEL_KERNEL);
		proc->type = G_THREAD_TYPE_SYSCALL;
		proc->syscall.sourceTask = caller;
		caller->syscall.processingTask = proc;
	} else
	{
		taskingResetTaskState(proc);
		proc->status = G_THREAD_STATUS_RUNNING;
	}
	proc->state->eip = (g_virtual_address) syscallThreadEntry;

	// Put task in scheduling
	taskingAssign(taskingGetLocal(), proc);
	taskingScheduleTo(proc);

	// Store call information
	caller->syscall.handler = handler;
	caller->syscall.data = syscallData;

	// Let caller task wait
	caller->status = G_THREAD_STATUS_WAITING;

	mutexRelease(&taskingGetLocal()->lock);
}

void syscallThreadEntry()
{
	g_task* sourceTask = taskingGetLocal()->current->syscall.sourceTask;

	// Call handler
	sourceTask->syscall.handler(sourceTask, sourceTask->syscall.data);

	// Switch back to source task
	mutexAcquire(&taskingGetLocal()->lock);
	sourceTask->status = G_THREAD_STATUS_RUNNING;
	taskingGetLocal()->current->status = G_THREAD_STATUS_UNUSED;
	taskingScheduleTo(sourceTask);
	mutexRelease(&taskingGetLocal()->lock);

	taskingKernelThreadYield();
}

void syscallRegister(int callId, g_syscall_handler handler, bool threaded)
{
	if(callId > G_SYSCALL_MAX)
	{
		kernelPanic("%! tried to register syscall with id %i, maximum is %i", "syscall", callId, G_SYSCALL_MAX);
	}

	syscallRegistrations[callId].handler = handler;
	syscallRegistrations[callId].threaded = threaded;
}

void syscallRegisterAll()
{
	syscallRegistrations = (g_syscall_registration*) heapAllocate(sizeof(g_syscall_registration) * G_SYSCALL_MAX);
	for(int i = 0; i < G_SYSCALL_MAX; i++)
	{
		syscallRegistrations[i].handler = 0;
	}

	syscallRegister(G_SYSCALL_SLEEP, (g_syscall_handler) syscallSleep, false);
	syscallRegister(G_SYSCALL_FS_READ, (g_syscall_handler) syscallFsRead, true);
}
