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
#include "kernel/kernel.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/tasking.hpp"

#include "kernel/calls/syscall_filesystem.hpp"
#include "kernel/calls/syscall_general.hpp"
#include "kernel/calls/syscall_memory.hpp"
#include "kernel/calls/syscall_messaging.hpp"
#include "kernel/calls/syscall_tasking.hpp"
#include "kernel/calls/syscall_vm86.hpp"

g_syscall_registration* syscallRegistrations = 0;

void syscallHandle(g_task* task)
{
    uint32_t callId = task->state->eax;
    void* syscallData = (void*) task->state->ebx;

    syscall(callId, syscallData);
}

void syscall(uint32_t callId, void* syscallData)
{
    if(callId > G_SYSCALL_MAX)
    {
        logInfo("%! tried to use out-of-range syscall %i", "syscall", callId);
        return;
    }

    g_syscall_registration* reg = &syscallRegistrations[callId];
    if(reg->handler == 0)
    {
        logInfo("%! tried to use unknown syscall %i", "syscall", callId);
        return;
    }

    reg->handler(taskingGetCurrentTask(), syscallData);
}

void syscallRegister(int callId, g_syscall_handler handler)
{
    if(callId > G_SYSCALL_MAX)
    {
        kernelPanic("%! tried to register syscall with id %i, maximum is %i", "syscall", callId, G_SYSCALL_MAX);
    }

    syscallRegistrations[callId].handler = handler;
}

void syscallRegisterAll()
{
    syscallRegistrations = (g_syscall_registration*) heapAllocate(sizeof(g_syscall_registration) * G_SYSCALL_MAX);
    for(int i = 0; i < G_SYSCALL_MAX; i++)
    {
        syscallRegistrations[i].handler = 0;
    }

    syscallRegister(G_SYSCALL_EXIT, (g_syscall_handler) syscallExit);
    syscallRegister(G_SYSCALL_YIELD, (g_syscall_handler) syscallYield);
    syscallRegister(G_SYSCALL_GET_PROCESS_ID, (g_syscall_handler) syscallGetProcessId);
    syscallRegister(G_SYSCALL_GET_TASK_ID, (g_syscall_handler) syscallGetTaskId);
    syscallRegister(G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID, (g_syscall_handler) syscallGetProcessIdForTaskId);
    syscallRegister(G_SYSCALL_FORK, (g_syscall_handler) syscallFork);
    syscallRegister(G_SYSCALL_JOIN, (g_syscall_handler) syscallJoin);
    syscallRegister(G_SYSCALL_SLEEP, (g_syscall_handler) syscallSleep);
    syscallRegister(G_SYSCALL_ATOMIC_LOCK, (g_syscall_handler) syscallAtomicLock);
    syscallRegister(G_SYSCALL_LOG, (g_syscall_handler) syscallLog);
    syscallRegister(G_SYSCALL_SET_VIDEO_LOG, (g_syscall_handler) syscallSetVideoLog);
    syscallRegister(G_SYSCALL_TEST, (g_syscall_handler) syscallTest);
    syscallRegister(G_SYSCALL_RELEASE_CLI_ARGUMENTS, (g_syscall_handler) syscallReleaseCliArguments);
    syscallRegister(G_SYSCALL_GET_WORKING_DIRECTORY, (g_syscall_handler) syscallGetWorkingDirectory);
    syscallRegister(G_SYSCALL_SET_WORKING_DIRECTORY, (g_syscall_handler) syscallSetWorkingDirectory);
    syscallRegister(G_SYSCALL_KILL, (g_syscall_handler) syscallKill);
    syscallRegister(G_SYSCALL_REGISTER_IRQ_HANDLER, (g_syscall_handler) syscallRegisterIrqHandler);
    syscallRegister(G_SYSCALL_RESTORE_INTERRUPTED_STATE, (g_syscall_handler) syscallRestoreInterruptedState);
    syscallRegister(G_SYSCALL_REGISTER_SIGNAL_HANDLER, (g_syscall_handler) syscallRegisterSignalHandler);
    syscallRegister(G_SYSCALL_RAISE_SIGNAL, (g_syscall_handler) syscallRaiseSignal);
    syscallRegister(G_SYSCALL_KERNQUERY, (g_syscall_handler) syscallKernQuery);
    syscallRegister(G_SYSCALL_GET_EXECUTABLE_PATH, (g_syscall_handler) syscallGetExecutablePath);
    syscallRegister(G_SYSCALL_GET_PARENT_PROCESS_ID, (g_syscall_handler) syscallGetParentProcessId);
    syscallRegister(G_SYSCALL_TASK_GET_TLS, (g_syscall_handler) syscallTaskGetTls);
    syscallRegister(G_SYSCALL_PROCESS_GET_INFO, (g_syscall_handler) syscallProcessGetInfo);

    syscallRegister(G_SYSCALL_CALL_VM86, (g_syscall_handler) syscallCallVm86);
    syscallRegister(G_SYSCALL_LOWER_MEMORY_ALLOCATE, (g_syscall_handler) syscallLowerMemoryAllocate);
    syscallRegister(G_SYSCALL_LOWER_MEMORY_FREE, (g_syscall_handler) syscallLowerMemoryFree);
    syscallRegister(G_SYSCALL_ALLOCATE_MEMORY, (g_syscall_handler) syscallAllocateMemory);
    syscallRegister(G_SYSCALL_UNMAP, (g_syscall_handler) syscallUnmap);
    syscallRegister(G_SYSCALL_SHARE_MEMORY, (g_syscall_handler) syscallShareMemory);
    syscallRegister(G_SYSCALL_MAP_MMIO_AREA, (g_syscall_handler) syscallMapMmioArea);
    syscallRegister(G_SYSCALL_SBRK, (g_syscall_handler) syscallSbrk);

    syscallRegister(G_SYSCALL_SPAWN, (g_syscall_handler) syscallSpawn);
    syscallRegister(G_SYSCALL_CREATE_THREAD, (g_syscall_handler) syscallCreateThread);
    syscallRegister(G_SYSCALL_GET_THREAD_ENTRY, (g_syscall_handler) syscallGetThreadEntry);

    syscallRegister(G_SYSCALL_REGISTER_TASK_IDENTIFIER, (g_syscall_handler) syscallRegisterTaskIdentifier);
    syscallRegister(G_SYSCALL_GET_TASK_FOR_IDENTIFIER, (g_syscall_handler) syscallGetTaskForIdentifier);
    syscallRegister(G_SYSCALL_MESSAGE_SEND, (g_syscall_handler) syscallMessageSend);
    syscallRegister(G_SYSCALL_MESSAGE_RECEIVE, (g_syscall_handler) syscallMessageReceive);

    syscallRegister(G_SYSCALL_GET_MILLISECONDS, (g_syscall_handler) syscallGetMilliseconds);

    syscallRegister(G_SYSCALL_FS_OPEN, (g_syscall_handler) syscallFsOpen);
    syscallRegister(G_SYSCALL_FS_SEEK, (g_syscall_handler) syscallFsSeek);
    syscallRegister(G_SYSCALL_FS_READ, (g_syscall_handler) syscallFsRead);
    syscallRegister(G_SYSCALL_FS_WRITE, (g_syscall_handler) syscallFsWrite);
    syscallRegister(G_SYSCALL_FS_CLOSE, (g_syscall_handler) syscallFsClose);
    syscallRegister(G_SYSCALL_FS_CLONEFD, (g_syscall_handler) syscallFsCloneFd);
    syscallRegister(G_SYSCALL_FS_LENGTH, (g_syscall_handler) syscallFsLength);
    syscallRegister(G_SYSCALL_FS_TELL, (g_syscall_handler) syscallFsTell);
    syscallRegister(G_SYSCALL_FS_STAT, (g_syscall_handler) syscallFsStat);
    syscallRegister(G_SYSCALL_FS_FSTAT, (g_syscall_handler) syscallFsFstat);
    syscallRegister(G_SYSCALL_FS_PIPE, (g_syscall_handler) syscallFsPipe);
}
