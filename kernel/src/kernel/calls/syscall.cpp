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

#include "kernel/calls/syscall.hpp"
#include "kernel/calls/syscall_filesystem.hpp"
#include "kernel/calls/syscall_system.hpp"
#include "kernel/calls/syscall_memory.hpp"
#include "kernel/calls/syscall_messaging.hpp"
#include "kernel/calls/syscall_tasking.hpp"
#include "kernel/calls/syscall_mutex.hpp"
#include "kernel/calls/syscall_kernquery.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/panic.hpp"
#include "shared/logger/logger.hpp"

#include <ghost/syscall.h>

g_syscall_registration* syscallRegistrations = 0;

void syscallHandle(g_task* task)
{
	uint32_t callId = task->state->eax;
	void* syscallData = (void*) task->state->ebx;

	syscall(callId, syscallData);
}


void syscall(uint32_t callId, void* syscallData)
{
	g_task* task = taskingGetCurrentTask();
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

	auto state = task->state;
	if(reg->interruptible)
		interruptsEnable();

	reg->handler(task, syscallData);
	asm volatile("" ::: "memory");

	interruptsDisable();
	task->state = state;
}

void _syscallRegister(int callId, g_syscall_handler handler, bool interruptible = false)
{
	if(callId > G_SYSCALL_MAX)
		panic("%! tried to register syscall with id %i, maximum is %i", "syscall", callId, G_SYSCALL_MAX);

	syscallRegistrations[callId].handler = handler;
	syscallRegistrations[callId].interruptible = interruptible;
}

void syscallRegisterAll()
{
	syscallRegistrations = (g_syscall_registration*) heapAllocateClear(sizeof(g_syscall_registration) * G_SYSCALL_MAX);

	// Tasking
	_syscallRegister(G_SYSCALL_EXIT, (g_syscall_handler) syscallExit);
	_syscallRegister(G_SYSCALL_YIELD, (g_syscall_handler) syscallYield);
	_syscallRegister(G_SYSCALL_GET_PROCESS_ID, (g_syscall_handler) syscallGetProcessId);
	_syscallRegister(G_SYSCALL_GET_TASK_ID, (g_syscall_handler) syscallGetTaskId);
	_syscallRegister(G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID, (g_syscall_handler) syscallGetProcessIdForTaskId);
	_syscallRegister(G_SYSCALL_FORK, (g_syscall_handler) syscallFork);
	_syscallRegister(G_SYSCALL_JOIN, (g_syscall_handler) syscallJoin);
	_syscallRegister(G_SYSCALL_SLEEP, (g_syscall_handler) syscallSleep);
	_syscallRegister(G_SYSCALL_RELEASE_CLI_ARGUMENTS, (g_syscall_handler) syscallReleaseCliArguments);
	_syscallRegister(G_SYSCALL_GET_WORKING_DIRECTORY, (g_syscall_handler) syscallGetWorkingDirectory);
	_syscallRegister(G_SYSCALL_SET_WORKING_DIRECTORY, (g_syscall_handler) syscallSetWorkingDirectory);
	_syscallRegister(G_SYSCALL_KILL, (g_syscall_handler) syscallKill);
	_syscallRegister(G_SYSCALL_GET_EXECUTABLE_PATH, (g_syscall_handler) syscallGetExecutablePath);
	_syscallRegister(G_SYSCALL_GET_PARENT_PROCESS_ID, (g_syscall_handler) syscallGetParentProcessId);
	_syscallRegister(G_SYSCALL_TASK_GET_TLS, (g_syscall_handler) syscallTaskGetTls);
	_syscallRegister(G_SYSCALL_PROCESS_GET_INFO, (g_syscall_handler) syscallProcessGetInfo);
	_syscallRegister(G_SYSCALL_SPAWN, (g_syscall_handler) syscallSpawn, true);
	_syscallRegister(G_SYSCALL_CREATE_TASK, (g_syscall_handler) syscallCreateTask);
	_syscallRegister(G_SYSCALL_GET_TASK_ENTRY, (g_syscall_handler) syscallGetTaskEntry);
	_syscallRegister(G_SYSCALL_EXIT_TASK, (g_syscall_handler) syscallExitTask);
	_syscallRegister(G_SYSCALL_REGISTER_TASK_IDENTIFIER, (g_syscall_handler) syscallRegisterTaskIdentifier);
	_syscallRegister(G_SYSCALL_GET_TASK_FOR_IDENTIFIER, (g_syscall_handler) syscallGetTaskForIdentifier);
	_syscallRegister(G_SYSCALL_GET_MILLISECONDS, (g_syscall_handler) syscallGetMilliseconds);
	_syscallRegister(G_SYSCALL_DUMP, (g_syscall_handler) syscallDump);

	// Memory
	_syscallRegister(G_SYSCALL_LOWER_MEMORY_ALLOCATE, (g_syscall_handler) syscallLowerMemoryAllocate, true);
	_syscallRegister(G_SYSCALL_LOWER_MEMORY_FREE, (g_syscall_handler) syscallLowerMemoryFree, true);
	_syscallRegister(G_SYSCALL_ALLOCATE_MEMORY, (g_syscall_handler) syscallAllocateMemory, true);
	_syscallRegister(G_SYSCALL_UNMAP, (g_syscall_handler) syscallUnmap, true);
	_syscallRegister(G_SYSCALL_SHARE_MEMORY, (g_syscall_handler) syscallShareMemory, true);
	_syscallRegister(G_SYSCALL_MAP_MMIO_AREA, (g_syscall_handler) syscallMapMmioArea, true);
	_syscallRegister(G_SYSCALL_SBRK, (g_syscall_handler) syscallSbrk, true);

	// Mutex
	_syscallRegister(G_SYSCALL_USER_MUTEX_INITIALIZE, (g_syscall_handler) syscallMutexInitialize);
	_syscallRegister(G_SYSCALL_USER_MUTEX_ACQUIRE, (g_syscall_handler) sycallMutexAcquire);
	_syscallRegister(G_SYSCALL_USER_MUTEX_RELEASE, (g_syscall_handler) syscallMutexRelease);
	_syscallRegister(G_SYSCALL_USER_MUTEX_DESTROY, (g_syscall_handler) syscallMutexDestroy);

	// Messages
	_syscallRegister(G_SYSCALL_MESSAGE_SEND, (g_syscall_handler) syscallMessageSend);
	_syscallRegister(G_SYSCALL_MESSAGE_RECEIVE, (g_syscall_handler) syscallMessageReceive);
	_syscallRegister(G_SYSCALL_MESSAGE_NEXT_TXID, (g_syscall_handler) syscallMessageNextTxid);

	// Filesystem
	_syscallRegister(G_SYSCALL_FS_OPEN, (g_syscall_handler) syscallFsOpen, true);
	_syscallRegister(G_SYSCALL_FS_SEEK, (g_syscall_handler) syscallFsSeek, true);
	_syscallRegister(G_SYSCALL_FS_READ, (g_syscall_handler) syscallFsRead, true);
	_syscallRegister(G_SYSCALL_FS_WRITE, (g_syscall_handler) syscallFsWrite, true);
	_syscallRegister(G_SYSCALL_FS_CLOSE, (g_syscall_handler) syscallFsClose, true);
	_syscallRegister(G_SYSCALL_FS_CLONEFD, (g_syscall_handler) syscallFsCloneFd, true);
	_syscallRegister(G_SYSCALL_FS_LENGTH, (g_syscall_handler) syscallFsLength, true);
	_syscallRegister(G_SYSCALL_FS_TELL, (g_syscall_handler) syscallFsTell, true);
	_syscallRegister(G_SYSCALL_FS_STAT, (g_syscall_handler) syscallFsStat, true);
	_syscallRegister(G_SYSCALL_FS_FSTAT, (g_syscall_handler) syscallFsFstat, true);
	_syscallRegister(G_SYSCALL_FS_PIPE, (g_syscall_handler) syscallFsPipe, true);
	_syscallRegister(G_SYSCALL_FS_OPEN_DIRECTORY, (g_syscall_handler) syscallFsOpenDirectory, true);
	_syscallRegister(G_SYSCALL_FS_READ_DIRECTORY, (g_syscall_handler) syscallFsReadDirectory, true);
	_syscallRegister(G_SYSCALL_FS_CLOSE_DIRECTORY, (g_syscall_handler) syscallFsCloseDirectory, true);
	_syscallRegister(G_SYSCALL_OPEN_IRQ_DEVICE, (g_syscall_handler) syscallOpenIrqDevice, true);
	_syscallRegister(G_SYSCALL_FS_REAL_PATH, (g_syscall_handler) syscallFsRealPath, true);

	// System
	_syscallRegister(G_SYSCALL_LOG, (g_syscall_handler) syscallLog);
	_syscallRegister(G_SYSCALL_SET_VIDEO_LOG, (g_syscall_handler) syscallSetVideoLog);
	_syscallRegister(G_SYSCALL_TEST, (g_syscall_handler) syscallTest);
	_syscallRegister(G_SYSCALL_CALL_VM86, (g_syscall_handler) syscallCallVm86);
	_syscallRegister(G_SYSCALL_IRQ_CREATE_REDIRECT, (g_syscall_handler) syscallIrqCreateRedirect);

	// Kernquery
	_syscallRegister(G_SYSCALL_KERNQUERY, (g_syscall_handler) syscallKernQuery);

}
