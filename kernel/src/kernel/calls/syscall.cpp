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
#include "kernel/calls/syscall_tasking.hpp"
#include "kernel/calls/syscall_memory.hpp"
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

	// Store call information
	task->syscall.handler = reg->handler;
	task->syscall.data = syscallData;

	if(reg->threaded)
		syscallRunThreaded(reg->handler, task, syscallData);
	else
		reg->handler(task, syscallData);
}

void syscallRunThreaded(g_syscall_handler handler, g_task* caller, void* syscallData)
{
	g_tasking_local* local = taskingGetLocal();
	mutexAcquire(&local->lock);

	g_task* proc = caller->syscall.processingTask;
	if(proc == 0)
	{
		proc = taskingCreateThread(0, caller->process, G_SECURITY_LEVEL_KERNEL);
		proc->type = G_THREAD_TYPE_SYSCALL;
		proc->syscall.sourceTask = caller;
		caller->syscall.processingTask = proc;
	}

	taskingResetTaskState(proc);
	proc->status = G_THREAD_STATUS_RUNNING;
	proc->state->eip = (g_virtual_address) syscallThreadEntry;

	// Let caller task wait
	caller->status = G_THREAD_STATUS_WAITING;

	// Put task in scheduling
	taskingAssign(local, proc);
	mutexRelease(&local->lock);

	taskingPleaseSchedule(proc);
	taskingSchedule();
}

void syscallThreadEntry()
{
	g_tasking_local* local = taskingGetLocal();
	g_task* sourceTask = local->scheduling.current->syscall.sourceTask;

	// Call handler
	sourceTask->syscall.handler(sourceTask, sourceTask->syscall.data);

	// Switch back to source task
	mutexAcquire(&local->lock);
	sourceTask->status = G_THREAD_STATUS_RUNNING;
	local->scheduling.current->status = G_THREAD_STATUS_UNUSED;
	mutexRelease(&local->lock);

	taskingPleaseSchedule(sourceTask);
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

	syscallRegister(G_SYSCALL_EXIT, (g_syscall_handler) syscallExit, false);
	syscallRegister(G_SYSCALL_YIELD, (g_syscall_handler) syscallYield, false);
	syscallRegister(G_SYSCALL_GET_PROCESS_ID, (g_syscall_handler) syscallGetProcessId, false);
	syscallRegister(G_SYSCALL_GET_TASK_ID, (g_syscall_handler) syscallGetTaskId, false);
	syscallRegister(G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID, (g_syscall_handler) syscallGetProcessIdForTaskId, false);
	syscallRegister(G_SYSCALL_FORK, (g_syscall_handler) syscallFork, false);
	syscallRegister(G_SYSCALL_JOIN, (g_syscall_handler) syscallJoin, false);
	syscallRegister(G_SYSCALL_SLEEP, (g_syscall_handler) syscallSleep, false);
	syscallRegister(G_SYSCALL_ATOMIC_LOCK, (g_syscall_handler) syscallAtomicLock, false);
	syscallRegister(G_SYSCALL_WAIT_FOR_IRQ, (g_syscall_handler) syscallWaitForIrq, false);
	syscallRegister(G_SYSCALL_LOG, (g_syscall_handler) syscallLog, false);
	syscallRegister(G_SYSCALL_SET_VIDEO_LOG, (g_syscall_handler) syscallSetVideoLog, false);
	syscallRegister(G_SYSCALL_TEST, (g_syscall_handler) syscallTest, false);
	syscallRegister(G_SYSCALL_RELEASE_CLI_ARGUMENTS, (g_syscall_handler) syscallReleaseCliArguments, false);

	syscallRegister(G_SYSCALL_REGISTER_SIGNAL_HANDLER, (g_syscall_handler) syscallRegisterSignalHandler, false);
	syscallRegister(G_SYSCALL_REGISTER_IRQ_HANDLER, (g_syscall_handler) syscallRegisterIrqHandler, false);
	syscallRegister(G_SYSCALL_RESTORE_INTERRUPTED_STATE, (g_syscall_handler) syscallRestoreInterruptedState, false);
	syscallRegister(G_SYSCALL_RAISE_SIGNAL, (g_syscall_handler) syscallRaiseSignal, false);

	syscallRegister(G_SYSCALL_GET_MILLISECONDS, (g_syscall_handler) syscallGetMilliseconds, false);

	syscallRegister(G_SYSCALL_SBRK, (g_syscall_handler) syscallSbrk, false);

	syscallRegister(G_SYSCALL_FS_READ, (g_syscall_handler) syscallFsRead, true);
}
