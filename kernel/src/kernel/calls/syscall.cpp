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
#include "ghost/calls/calls.h"
#include "kernel/calls/syscall_filesystem.hpp"
#include "kernel/calls/syscall_general.hpp"
#include "kernel/calls/syscall_memory.hpp"
#include "kernel/calls/syscall_messaging.hpp"
#include "kernel/calls/syscall_tasking.hpp"
#include "kernel/calls/syscall_vm86.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/tasking/tasking.hpp"
#include "shared/panic.hpp"

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

	volatile g_processor_state* state;
	if(reg->reentrant)
	{
		state = task->state;
		interruptsEnable();
	}

	reg->handler(task, syscallData);

	if(reg->reentrant)
	{
		interruptsDisable();
		task->state = state;
	}
}

void _syscallRegister(int callId, g_syscall_handler handler, bool reentrant)
{
	if(callId > G_SYSCALL_MAX)
	{
		panic("%! tried to register syscall with id %i, maximum is %i", "syscall", callId, G_SYSCALL_MAX);
	}

	syscallRegistrations[callId].handler = handler;
	syscallRegistrations[callId].reentrant = reentrant;
}

void syscallRegisterAll()
{
	syscallRegistrations = (g_syscall_registration*) heapAllocateClear(sizeof(g_syscall_registration) * G_SYSCALL_MAX);

	_syscallRegister(G_SYSCALL_EXIT, (g_syscall_handler) syscallExit, false);
	_syscallRegister(G_SYSCALL_YIELD, (g_syscall_handler) syscallYield, false);
	_syscallRegister(G_SYSCALL_GET_PROCESS_ID, (g_syscall_handler) syscallGetProcessId, false);
	_syscallRegister(G_SYSCALL_GET_TASK_ID, (g_syscall_handler) syscallGetTaskId, false);
	_syscallRegister(G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID, (g_syscall_handler) syscallGetProcessIdForTaskId, false);
	_syscallRegister(G_SYSCALL_FORK, (g_syscall_handler) syscallFork, false);
	_syscallRegister(G_SYSCALL_JOIN, (g_syscall_handler) syscallJoin, false);
	_syscallRegister(G_SYSCALL_SLEEP, (g_syscall_handler) syscallSleep, false);
	_syscallRegister(G_SYSCALL_ATOMIC_INITIALIZE, (g_syscall_handler) syscallAtomicInitialize, false);
	_syscallRegister(G_SYSCALL_ATOMIC_LOCK, (g_syscall_handler) syscallAtomicLock, false);
	_syscallRegister(G_SYSCALL_ATOMIC_UNLOCK, (g_syscall_handler) syscallAtomicUnlock, false);
	_syscallRegister(G_SYSCALL_ATOMIC_DESTROY, (g_syscall_handler) syscallAtomicDestroy, false);
	_syscallRegister(G_SYSCALL_LOG, (g_syscall_handler) syscallLog, false);
	_syscallRegister(G_SYSCALL_SET_VIDEO_LOG, (g_syscall_handler) syscallSetVideoLog, false);
	_syscallRegister(G_SYSCALL_TEST, (g_syscall_handler) syscallTest, false);
	_syscallRegister(G_SYSCALL_RELEASE_CLI_ARGUMENTS, (g_syscall_handler) syscallReleaseCliArguments, false);
	_syscallRegister(G_SYSCALL_GET_WORKING_DIRECTORY, (g_syscall_handler) syscallGetWorkingDirectory, false);
	_syscallRegister(G_SYSCALL_SET_WORKING_DIRECTORY, (g_syscall_handler) syscallSetWorkingDirectory, false);
	_syscallRegister(G_SYSCALL_KILL, (g_syscall_handler) syscallKill, false);
	_syscallRegister(G_SYSCALL_OPEN_IRQ_DEVICE, (g_syscall_handler) syscallOpenIrqDevice, false);
	_syscallRegister(G_SYSCALL_KERNQUERY, (g_syscall_handler) syscallKernQuery, true);
	_syscallRegister(G_SYSCALL_GET_EXECUTABLE_PATH, (g_syscall_handler) syscallGetExecutablePath, false);
	_syscallRegister(G_SYSCALL_GET_PARENT_PROCESS_ID, (g_syscall_handler) syscallGetParentProcessId, false);
	_syscallRegister(G_SYSCALL_TASK_GET_TLS, (g_syscall_handler) syscallTaskGetTls, false);
	_syscallRegister(G_SYSCALL_PROCESS_GET_INFO, (g_syscall_handler) syscallProcessGetInfo, false);

	_syscallRegister(G_SYSCALL_CALL_VM86, (g_syscall_handler) syscallCallVm86, false);
	_syscallRegister(G_SYSCALL_LOWER_MEMORY_ALLOCATE, (g_syscall_handler) syscallLowerMemoryAllocate, true);
	_syscallRegister(G_SYSCALL_LOWER_MEMORY_FREE, (g_syscall_handler) syscallLowerMemoryFree, true);
	_syscallRegister(G_SYSCALL_ALLOCATE_MEMORY, (g_syscall_handler) syscallAllocateMemory, true);
	_syscallRegister(G_SYSCALL_UNMAP, (g_syscall_handler) syscallUnmap, true);
	_syscallRegister(G_SYSCALL_SHARE_MEMORY, (g_syscall_handler) syscallShareMemory, true);
	_syscallRegister(G_SYSCALL_MAP_MMIO_AREA, (g_syscall_handler) syscallMapMmioArea, true);
	_syscallRegister(G_SYSCALL_SBRK, (g_syscall_handler) syscallSbrk, true);

	_syscallRegister(G_SYSCALL_SPAWN, (g_syscall_handler) syscallSpawn, true);
	_syscallRegister(G_SYSCALL_CREATE_THREAD, (g_syscall_handler) syscallCreateThread, false);
	_syscallRegister(G_SYSCALL_GET_THREAD_ENTRY, (g_syscall_handler) syscallGetThreadEntry, false);
	_syscallRegister(G_SYSCALL_EXIT_THREAD, (g_syscall_handler) syscallExitThread, false);

	_syscallRegister(G_SYSCALL_REGISTER_TASK_IDENTIFIER, (g_syscall_handler) syscallRegisterTaskIdentifier, false);
	_syscallRegister(G_SYSCALL_GET_TASK_FOR_IDENTIFIER, (g_syscall_handler) syscallGetTaskForIdentifier, false);
	_syscallRegister(G_SYSCALL_MESSAGE_SEND, (g_syscall_handler) syscallMessageSend, false);
	_syscallRegister(G_SYSCALL_MESSAGE_RECEIVE, (g_syscall_handler) syscallMessageReceive, false);

	_syscallRegister(G_SYSCALL_GET_MILLISECONDS, (g_syscall_handler) syscallGetMilliseconds, false);

	_syscallRegister(G_SYSCALL_FS_OPEN, (g_syscall_handler) syscallFsOpen, false);
	_syscallRegister(G_SYSCALL_FS_SEEK, (g_syscall_handler) syscallFsSeek, false);
	_syscallRegister(G_SYSCALL_FS_READ, (g_syscall_handler) syscallFsRead, false);
	_syscallRegister(G_SYSCALL_FS_WRITE, (g_syscall_handler) syscallFsWrite, false);
	_syscallRegister(G_SYSCALL_FS_CLOSE, (g_syscall_handler) syscallFsClose, false);
	_syscallRegister(G_SYSCALL_FS_CLONEFD, (g_syscall_handler) syscallFsCloneFd, false);
	_syscallRegister(G_SYSCALL_FS_LENGTH, (g_syscall_handler) syscallFsLength, false);
	_syscallRegister(G_SYSCALL_FS_TELL, (g_syscall_handler) syscallFsTell, false);
	_syscallRegister(G_SYSCALL_FS_STAT, (g_syscall_handler) syscallFsStat, false);
	_syscallRegister(G_SYSCALL_FS_FSTAT, (g_syscall_handler) syscallFsFstat, false);
	_syscallRegister(G_SYSCALL_FS_PIPE, (g_syscall_handler) syscallFsPipe, false);
	_syscallRegister(G_SYSCALL_FS_OPEN_DIRECTORY, (g_syscall_handler) syscallFsOpenDirectory, false);
	_syscallRegister(G_SYSCALL_FS_READ_DIRECTORY, (g_syscall_handler) syscallFsReadDirectory, false);
	_syscallRegister(G_SYSCALL_FS_CLOSE_DIRECTORY, (g_syscall_handler) syscallFsCloseDirectory, false);
}
