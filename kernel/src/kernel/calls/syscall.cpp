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
#include "kernel/calls/syscall_kernquery.hpp"
#include "kernel/calls/syscall_memory.hpp"
#include "kernel/calls/syscall_messaging.hpp"
#include "kernel/calls/syscall_mutex.hpp"
#include "kernel/calls/syscall_system.hpp"
#include "kernel/calls/syscall_tasking.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/memory/memory_user.hpp"
#include "kernel/panic.hpp"
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/power/power.hpp"
#include "kernel/tasking/tasking.hpp"

#include <ghost/syscall.h>

g_syscall_registration* syscallRegistrations = nullptr;

bool syscallStartsTask(uint32_t call) { return call == G_SYSCALL_SPAWN || call == G_SYSCALL_CREATE_TASK; }

bool syscallMutatesState(uint32_t call)
{
	switch(call)
	{
		case G_SYSCALL_SPAWN:
		case G_SYSCALL_CREATE_TASK:
		case G_SYSCALL_FS_OPEN:
		case G_SYSCALL_FS_WRITE:
		case G_SYSCALL_FS_CLOSE:
		case G_SYSCALL_FS_CREATE_DIRECTORY:
		case G_SYSCALL_FS_UNLINK:
		case G_SYSCALL_FS_UNLINK_DIRECTORY:
		case G_SYSCALL_FS_TRUNCATE:
		case G_SYSCALL_FS_SYMLINK:
		case G_SYSCALL_FS_RENAME:
		case G_SYSCALL_FS_SYNC:
		case G_SYSCALL_FS_DELEGATE_REGISTER:
		case G_SYSCALL_FS_DELEGATE_UNREGISTER:
			return true;
		default:
			return false;
	}
}

void syscallHandle(g_task* task)
{
	uint64_t callId = task->state->rax;
	void* syscallData = (void*) task->state->rdi;
	if(callId > UINT32_MAX)
		return;

	syscall(callId, syscallData);
}


/**
 * Copies requests into kernel storage and retains response pages before the
 * operation. Pointer fields require separate checked transfers in the handler.
 */
static void _syscallInvoke(g_task* task, const g_syscall_registration* registration, void* address)
{
	auto size = registration->requestSize;
	void* request = address;
	g_user_memory* response = nullptr;

	uint8_t data[G_SYSCALL_MAX_REQUEST_SIZE] = {};
	if(size && task->securityLevel != G_SECURITY_LEVEL_KERNEL)
	{
		auto access = registration->access;
		bool copyInput = access == G_SYSCALL_INPUT || access == G_SYSCALL_IN_OUT;
		bool copyOutput = access == G_SYSCALL_OUTPUT || access == G_SYSCALL_IN_OUT;

		if(copyOutput)
		{
			response = memoryUserAcquire(task, (g_address) address, size, true);
			if(!response)
				return;
		}

		if(copyInput && !memoryCopyFromUser(task, data, (g_address) address, size))
			return;
		request = data;
	}

	registration->handler(task, request);

	if(response)
		memoryUserWrite(response, data, size);
}

void syscall(uint32_t callId, void* syscallData)
{
	taskingHandleTerminationRequest();

	auto task = taskingGetCurrentTask();
	if(callId >= G_SYSCALL_MAX)
	{
		logInfo("%! task %i tried to use out-of-range syscall %i", "syscall", task->id, callId);
		return;
	}

	g_syscall_registration* reg = &syscallRegistrations[callId];
	if(reg->handler == nullptr)
	{
		logInfo("%! task %i tried to use unknown syscall %i", "syscall", task->id, callId);
		return;
	}

	auto state = task->state;
	if(reg->interruptible)
		interruptsEnable();

	auto powerTracked = powerSyscallEnter(callId);

	_syscallInvoke(task, reg, syscallData);

	memoryUserReleaseAll(task);
	powerSyscallLeave(powerTracked);
	asm volatile("" ::: "memory");

	interruptsDisable();
	task->state = state;

	taskingHandleTerminationRequest();
}

/**
 * Registers a handler and the size and access direction of its request.
 * Rejects requests that cannot fit the dispatcher's local storage.
 */
static void _syscallRegister(int callId, g_syscall_handler handler, size_t requestSize = 0,
							 g_syscall_access access = G_SYSCALL_IN_OUT, bool interruptible = false)
{
	if(callId < 0 || callId >= G_SYSCALL_MAX)
		panic("%! tried to register syscall with id %i, maximum is %i", "syscall", callId, G_SYSCALL_MAX);
	if(requestSize > G_SYSCALL_MAX_REQUEST_SIZE)
		panic("%! request for syscall %i exceeds the maximum size", "syscall", callId);

	auto registration = &syscallRegistrations[callId];
	registration->handler = handler;
	registration->interruptible = interruptible;
	registration->requestSize = requestSize;
	registration->access = access;
}

void syscallRegisterAll()
{
	syscallRegistrations = (g_syscall_registration*) heapAllocateClear(sizeof(g_syscall_registration) * G_SYSCALL_MAX);

	// Tasking
	_syscallRegister(G_SYSCALL_EXIT, (g_syscall_handler) syscallExit);
	_syscallRegister(G_SYSCALL_YIELD, (g_syscall_handler) syscallYield, sizeof(g_syscall_yield), G_SYSCALL_INPUT);
	_syscallRegister(G_SYSCALL_GET_PROCESS_ID, (g_syscall_handler) syscallGetProcessId, sizeof(g_syscall_get_pid), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_GET_TASK_ID, (g_syscall_handler) syscallGetTaskId, sizeof(g_syscall_get_tid), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID, (g_syscall_handler) syscallGetProcessIdForTaskId, sizeof(g_syscall_get_pid_for_tid), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_FORK, (g_syscall_handler) syscallFork);
	_syscallRegister(G_SYSCALL_JOIN, (g_syscall_handler) syscallJoin, sizeof(g_syscall_join), G_SYSCALL_INPUT);
	_syscallRegister(G_SYSCALL_SLEEP, (g_syscall_handler) syscallSleep, sizeof(g_syscall_sleep), G_SYSCALL_INPUT);
	_syscallRegister(G_SYSCALL_RELEASE_CLI_ARGUMENTS, (g_syscall_handler) syscallReleaseCliArguments, sizeof(g_syscall_cli_args_release), G_SYSCALL_INPUT);
	_syscallRegister(G_SYSCALL_GET_WORKING_DIRECTORY, (g_syscall_handler) syscallGetWorkingDirectory, sizeof(g_syscall_get_working_directory), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_SET_WORKING_DIRECTORY, (g_syscall_handler) syscallSetWorkingDirectory, sizeof(g_syscall_set_working_directory), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_KILL, (g_syscall_handler) syscallKill, sizeof(g_syscall_kill), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_GET_EXECUTABLE_PATH, (g_syscall_handler) syscallGetExecutablePath, sizeof(g_syscall_get_executable_path), G_SYSCALL_INPUT);
	_syscallRegister(G_SYSCALL_GET_PARENT_PROCESS_ID, (g_syscall_handler) syscallGetParentProcessId);
	_syscallRegister(G_SYSCALL_TASK_GET_TLS, (g_syscall_handler) syscallTaskGetTls, sizeof(g_syscall_task_get_tls), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_PROCESS_GET_INFO, (g_syscall_handler) syscallProcessGetInfo, sizeof(g_syscall_process_get_info), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_SPAWN, (g_syscall_handler) syscallSpawn, sizeof(g_syscall_spawn), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_GET_LAUNCH_MODE, (g_syscall_handler) syscallGetLaunchMode, sizeof(g_syscall_get_launch_mode), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_CREATE_TASK, (g_syscall_handler) syscallCreateTask, sizeof(g_syscall_create_task), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_GET_TASK_ENTRY, (g_syscall_handler) syscallGetTaskEntry, sizeof(g_syscall_get_task_entry), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_EXIT_TASK, (g_syscall_handler) syscallExitTask);
	_syscallRegister(G_SYSCALL_TASK_REGISTER_NAME, (g_syscall_handler) syscallTaskRegisterName, sizeof(g_syscall_task_register_name), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_GET_TASK_BY_NAME, (g_syscall_handler) syscallGetTaskByName, sizeof(g_syscall_task_get_by_name), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_GET_MILLISECONDS, (g_syscall_handler) syscallGetMilliseconds, sizeof(g_syscall_millis), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_DUMP, (g_syscall_handler) syscallDump);
	_syscallRegister(G_SYSCALL_GET_NANOSECONDS, (g_syscall_handler) syscallGetNanoseconds, sizeof(g_syscall_nanos), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_GET_TIME, (g_syscall_handler) syscallGetTime, sizeof(g_syscall_time), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_TASK_AWAIT_BY_NAME, (g_syscall_handler) syscallTaskAwaitByName, sizeof(g_syscall_task_await_by_name), G_SYSCALL_IN_OUT);

	// Memory
	_syscallRegister(G_SYSCALL_LOWER_MEMORY_ALLOCATE, (g_syscall_handler) syscallLowerMemoryAllocate, sizeof(g_syscall_lower_malloc), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_LOWER_MEMORY_FREE, (g_syscall_handler) syscallLowerMemoryFree, sizeof(g_syscall_lower_free), G_SYSCALL_INPUT, true);
	_syscallRegister(G_SYSCALL_ALLOCATE_MEMORY, (g_syscall_handler) syscallAllocateMemory, sizeof(g_syscall_alloc_mem), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_UNMAP, (g_syscall_handler) syscallUnmap, sizeof(g_syscall_unmap), G_SYSCALL_INPUT, true);
	_syscallRegister(G_SYSCALL_SHARE_MEMORY, (g_syscall_handler) syscallShareMemory, sizeof(g_syscall_share_mem), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_MAP_MMIO_AREA, (g_syscall_handler) syscallMapMmioArea, sizeof(g_syscall_map_mmio), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_SBRK, (g_syscall_handler) syscallSbrk, sizeof(g_syscall_sbrk), G_SYSCALL_IN_OUT, true);

	// Mutex
	_syscallRegister(G_SYSCALL_USER_MUTEX_INITIALIZE, (g_syscall_handler) syscallMutexInitialize, sizeof(g_syscall_user_mutex_initialize), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_USER_MUTEX_ACQUIRE, (g_syscall_handler) sycallMutexAcquire, sizeof(g_syscall_user_mutex_acquire), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_USER_MUTEX_RELEASE, (g_syscall_handler) syscallMutexRelease, sizeof(g_syscall_user_mutex_release), G_SYSCALL_INPUT);
	_syscallRegister(G_SYSCALL_USER_MUTEX_DESTROY, (g_syscall_handler) syscallMutexDestroy, sizeof(g_syscall_user_mutex_destroy), G_SYSCALL_INPUT);

	// Messages
	_syscallRegister(G_SYSCALL_MESSAGE_SEND, (g_syscall_handler) syscallMessageSend, sizeof(g_syscall_send_message), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_MESSAGE_RECEIVE, (g_syscall_handler) syscallMessageReceive, sizeof(g_syscall_receive_message), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_MESSAGE_NEXT_TXID, (g_syscall_handler) syscallMessageNextTxId, sizeof(g_syscall_message_next_txid), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_MESSAGE_TOPIC_SEND, (g_syscall_handler) syscallMessageTopicSend, sizeof(g_syscall_send_topic_message), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_MESSAGE_TOPIC_RECEIVE, (g_syscall_handler) syscallMessageTopicReceive, sizeof(g_syscall_receive_topic_message), G_SYSCALL_IN_OUT);

	// Filesystem
	_syscallRegister(G_SYSCALL_FS_OPEN, (g_syscall_handler) syscallFsOpen, sizeof(g_syscall_fs_open), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_SEEK, (g_syscall_handler) syscallFsSeek, sizeof(g_syscall_fs_seek), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_READ, (g_syscall_handler) syscallFsRead, sizeof(g_syscall_fs_read), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_WRITE, (g_syscall_handler) syscallFsWrite, sizeof(g_syscall_fs_write), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_CLOSE, (g_syscall_handler) syscallFsClose, sizeof(g_syscall_fs_close), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_CLONEFD, (g_syscall_handler) syscallFsCloneFd, sizeof(g_syscall_fs_clonefd), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_LENGTH, (g_syscall_handler) syscallFsLength, sizeof(g_syscall_fs_length), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_TELL, (g_syscall_handler) syscallFsTell, sizeof(g_syscall_fs_tell), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_STAT, (g_syscall_handler) syscallFsStat, sizeof(g_syscall_fs_stat), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_FSTAT, (g_syscall_handler) syscallFsFstat, sizeof(g_syscall_fs_fstat), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_PIPE, (g_syscall_handler) syscallFsPipe, sizeof(g_syscall_fs_pipe), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_OPEN_DIRECTORY, (g_syscall_handler) syscallFsOpenDirectory, sizeof(g_syscall_fs_open_directory), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_READ_DIRECTORY, (g_syscall_handler) syscallFsReadDirectory, sizeof(g_syscall_fs_read_directory), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_CLOSE_DIRECTORY, (g_syscall_handler) syscallFsCloseDirectory, 0, G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_REAL_PATH, (g_syscall_handler) syscallFsRealPath, sizeof(g_syscall_fs_real_path), G_SYSCALL_IN_OUT, true);

	_syscallRegister(G_SYSCALL_FS_DELEGATE_REGISTER, (g_syscall_handler) syscallFsDelegateRegister, sizeof(g_syscall_fs_delegate_register), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_DELEGATE_TRANSACTION_COMPLETE, (g_syscall_handler) syscallFsDelegateTransactionComplete, sizeof(g_syscall_fs_delegate_transaction_complete), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_DELEGATE_NODE_CREATE, (g_syscall_handler) syscallFsDelegateNodeCreate, sizeof(g_syscall_fs_delegate_node_create), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_DELEGATE_UNREGISTER, (g_syscall_handler) syscallFsDelegateUnregister, sizeof(g_syscall_fs_delegate_unregister), G_SYSCALL_IN_OUT, true);

	_syscallRegister(G_SYSCALL_FS_CREATE_DIRECTORY, (g_syscall_handler) syscallFsCreateDirectory, sizeof(g_syscall_fs_path_mutation), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_UNLINK, (g_syscall_handler) syscallFsUnlink, sizeof(g_syscall_fs_path_mutation), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_UNLINK_DIRECTORY, (g_syscall_handler) syscallFsUnlinkDirectory, sizeof(g_syscall_fs_path_mutation), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_TRUNCATE, (g_syscall_handler) syscallFsTruncate, sizeof(g_syscall_fs_fd_mutation), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_SYMLINK, (g_syscall_handler) syscallFsSymlink, sizeof(g_syscall_fs_symlink), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_READLINK, (g_syscall_handler) syscallFsReadlink, sizeof(g_syscall_fs_readlink), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_RENAME, (g_syscall_handler) syscallFsRename, sizeof(g_syscall_fs_rename), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_FS_SYNC, (g_syscall_handler) syscallFsSync, sizeof(g_syscall_fs_fd_mutation), G_SYSCALL_IN_OUT, true);

	// System
	_syscallRegister(G_SYSCALL_LOG, (g_syscall_handler) syscallLog, sizeof(g_syscall_log), G_SYSCALL_INPUT);
	_syscallRegister(G_SYSCALL_OPEN_LOG_PIPE, (g_syscall_handler) syscallOpenLogPipe, sizeof(g_syscall_open_log_pipe), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_SET_VIDEO_LOG, (g_syscall_handler) syscallSetVideoLog, sizeof(g_syscall_set_video_log), G_SYSCALL_INPUT);
	_syscallRegister(G_SYSCALL_TEST, (g_syscall_handler) syscallTest, sizeof(g_syscall_test), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_CALL_VM86, (g_syscall_handler) syscallCallVm86, sizeof(g_syscall_call_vm86), G_SYSCALL_IN_OUT);
	_syscallRegister(G_SYSCALL_GET_EFI_FRAMEBUFFER, (g_syscall_handler) syscallGetEfiFramebuffer, sizeof(g_syscall_get_efi_framebuffer), G_SYSCALL_OUTPUT);
	_syscallRegister(G_SYSCALL_SYSTEM_POWER, (g_syscall_handler) syscallSystemPower, sizeof(g_syscall_system_power), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_IRQ, (g_syscall_handler) syscallIrq, sizeof(g_syscall_irq), G_SYSCALL_IN_OUT, true);
	_syscallRegister(G_SYSCALL_ACPI_GET_PLATFORM_INFO, (g_syscall_handler) syscallAcpiGetPlatformInfo, sizeof(g_syscall_acpi_platform_info), G_SYSCALL_OUTPUT, true);
	_syscallRegister(G_SYSCALL_ACPI_MAP, (g_syscall_handler) syscallAcpiMap, sizeof(g_syscall_acpi_map), G_SYSCALL_IN_OUT, true);

	// Kernquery
	_syscallRegister(G_SYSCALL_KERNQUERY, (g_syscall_handler) syscallKernQuery, sizeof(g_syscall_kernquery), G_SYSCALL_IN_OUT);
}
