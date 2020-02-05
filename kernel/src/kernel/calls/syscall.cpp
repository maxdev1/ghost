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
#include "kernel/calls/syscall_filesystem.hpp"
#include "kernel/calls/syscall_vm86.hpp"
#include "kernel/calls/syscall_messaging.hpp"

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

	g_task* thread = caller->syscall.processingTask;
	if(thread == 0)
	{
		thread = taskingCreateThread(0, caller->process, G_SECURITY_LEVEL_KERNEL);
		thread->type = G_THREAD_TYPE_SYSCALL;
		thread->syscall.sourceTask = caller;
		caller->syscall.processingTask = thread;
	}

	thread->state = (g_processor_state*) (thread->stack.end - sizeof(g_processor_state));
	taskingResetTaskState(thread);
	thread->status = G_THREAD_STATUS_RUNNING;
	thread->state->eip = (g_virtual_address) syscallThreadEntry;

	// Let caller task wait
	caller->status = G_THREAD_STATUS_WAITING;

	// Put task in scheduling
	taskingAssign(local, thread);
	mutexRelease(&local->lock);

	taskingPleaseSchedule(thread);
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
	syscallRegister(G_SYSCALL_LOG, (g_syscall_handler) syscallLog, false);
	syscallRegister(G_SYSCALL_SET_VIDEO_LOG, (g_syscall_handler) syscallSetVideoLog, false);
	syscallRegister(G_SYSCALL_TEST, (g_syscall_handler) syscallTest, false);
	syscallRegister(G_SYSCALL_RELEASE_CLI_ARGUMENTS, (g_syscall_handler) syscallReleaseCliArguments, false);
	syscallRegister(G_SYSCALL_GET_WORKING_DIRECTORY, (g_syscall_handler) syscallGetWorkingDirectory, false);
	syscallRegister(G_SYSCALL_SET_WORKING_DIRECTORY, (g_syscall_handler) syscallSetWorkingDirectory, false);
	syscallRegister(G_SYSCALL_KILL, (g_syscall_handler) syscallKill, false);
	syscallRegister(G_SYSCALL_REGISTER_IRQ_HANDLER, (g_syscall_handler) syscallRegisterIrqHandler, false);
	syscallRegister(G_SYSCALL_RESTORE_INTERRUPTED_STATE, (g_syscall_handler) syscallRestoreInterruptedState, false);
	syscallRegister(G_SYSCALL_REGISTER_SIGNAL_HANDLER, (g_syscall_handler) syscallRegisterSignalHandler, false);
	syscallRegister(G_SYSCALL_RAISE_SIGNAL, (g_syscall_handler) syscallRaiseSignal, false);
	syscallRegister(G_SYSCALL_KERNQUERY, (g_syscall_handler) syscallKernQuery, false);
	syscallRegister(G_SYSCALL_GET_EXECUTABLE_PATH, (g_syscall_handler) syscallGetExecutablePath, false);
	syscallRegister(G_SYSCALL_GET_PARENT_PROCESS_ID, (g_syscall_handler) syscallGetParentProcessId, false);
	syscallRegister(G_SYSCALL_TASK_GET_TLS, (g_syscall_handler) syscallTaskGetTls, false);
	syscallRegister(G_SYSCALL_PROCESS_GET_INFO, (g_syscall_handler) syscallProcessGetInfo, false);

	syscallRegister(G_SYSCALL_CALL_VM86, (g_syscall_handler) syscallCallVm86, false);
	syscallRegister(G_SYSCALL_LOWER_MEMORY_ALLOCATE, (g_syscall_handler) syscallLowerMemoryAllocate, false);
	syscallRegister(G_SYSCALL_LOWER_MEMORY_FREE, (g_syscall_handler) syscallLowerMemoryFree, false);
	syscallRegister(G_SYSCALL_ALLOCATE_MEMORY, (g_syscall_handler) syscallAllocateMemory, true);
	syscallRegister(G_SYSCALL_UNMAP, (g_syscall_handler) syscallUnmap, true);
	syscallRegister(G_SYSCALL_SHARE_MEMORY, (g_syscall_handler) syscallShareMemory, true);
	syscallRegister(G_SYSCALL_MAP_MMIO_AREA, (g_syscall_handler) syscallMapMmioArea, true);
	syscallRegister(G_SYSCALL_SBRK, (g_syscall_handler) syscallSbrk, false);

	syscallRegister(G_SYSCALL_SPAWN, (g_syscall_handler) syscallSpawn, true);
	syscallRegister(G_SYSCALL_CREATE_THREAD, (g_syscall_handler) syscallCreateThread, false);
	syscallRegister(G_SYSCALL_GET_THREAD_ENTRY, (g_syscall_handler) syscallGetThreadEntry, false);
	
	syscallRegister(G_SYSCALL_REGISTER_TASK_IDENTIFIER, (g_syscall_handler) syscallRegisterTaskIdentifier, false);
	syscallRegister(G_SYSCALL_GET_TASK_FOR_IDENTIFIER, (g_syscall_handler) syscallGetTaskForIdentifier, false);
	syscallRegister(G_SYSCALL_MESSAGE_SEND, (g_syscall_handler) syscallMessageSend, false);
	syscallRegister(G_SYSCALL_MESSAGE_RECEIVE, (g_syscall_handler) syscallMessageReceive, false);

	syscallRegister(G_SYSCALL_GET_MILLISECONDS, (g_syscall_handler) syscallGetMilliseconds, false);

	syscallRegister(G_SYSCALL_FS_OPEN, (g_syscall_handler) syscallFsOpen, true);
	syscallRegister(G_SYSCALL_FS_SEEK, (g_syscall_handler) syscallFsSeek, true);
	syscallRegister(G_SYSCALL_FS_READ, (g_syscall_handler) syscallFsRead, true);
	syscallRegister(G_SYSCALL_FS_WRITE, (g_syscall_handler) syscallFsWrite, true);
	syscallRegister(G_SYSCALL_FS_CLOSE, (g_syscall_handler) syscallFsClose, false);
	syscallRegister(G_SYSCALL_FS_CLONEFD, (g_syscall_handler) syscallFsCloneFd, false);
	syscallRegister(G_SYSCALL_FS_LENGTH, (g_syscall_handler) syscallFsLength, true);
	syscallRegister(G_SYSCALL_FS_TELL, (g_syscall_handler) syscallFsTell, false);
	syscallRegister(G_SYSCALL_FS_STAT, (g_syscall_handler) syscallFsStat, true);
	syscallRegister(G_SYSCALL_FS_FSTAT, (g_syscall_handler) syscallFsFstat, true);
	syscallRegister(G_SYSCALL_FS_PIPE, (g_syscall_handler) syscallFsPipe, true);
}

