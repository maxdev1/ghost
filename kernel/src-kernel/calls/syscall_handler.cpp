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
#include <calls/syscall_handler.hpp>

#include <kernel.hpp>
#include <build_config.hpp>
#include <ramdisk/ramdisk.hpp>
#include <executable/elf32_loader.hpp>
#include <logger/logger.hpp>
#include <tasking/tasking.hpp>
#include <tasking/thread_manager.hpp>
#include <tasking/communication/message_controller.hpp>
#include <system/interrupts/handling/interrupt_request_handler.hpp>
#include <memory/physical/pp_allocator.hpp>
#include <memory/address_space.hpp>
#include <memory/temporary_paging_util.hpp>
#include <memory/constants.hpp>
#include <memory/lower_heap.hpp>

// In debugging mode, make full output with system call name, identifier and process/thread id
#if G_DEBUG_SYSCALLS
#define link(val, func)																	\
	case val: {																			\
		g_thread* thr = g_tasking::getCurrentThread();									\
		const char* pident = thr->process->main->getIdentifier();						\
		const char* tident = thr->getIdentifier();										\
		g_log_info(#val " ('%s' %i:'%s' %i)", (pident == 0 ? "" : pident),				\
			thr->process->main->id, (tident == 0 ? "" : tident), thr->id);				\
		return func(state);																\
	}

#else
// In normal mode, only call handler
#define link(val, func)																	\
	case val: return func(state);
#endif

/**
 * The system call handler is called by the RequestHandler when a system call is executed.
 * The handler then checks which call was requested by taking it's number from
 * the callers EAX and redirects to the specific system call function.
 */
G_SYSCALL_HANDLER(handle) {

	uint32_t call = G_SYSCALL_CODE(state);

	switch (call) {
		link(G_SYSCALL_YIELD, yield);
		link(G_SYSCALL_EXIT, exit);
		link(G_SYSCALL_GET_PROCESS_ID, get_pid);
		link(G_SYSCALL_GET_TASK_ID, get_tid);
		link(G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID, get_pid_for_tid);
		link(G_SYSCALL_RAMDISK_SPAWN, ramdisk_spawn);
		link(G_SYSCALL_SLEEP, sleep);
		link(G_SYSCALL_CREATE_THREAD, create_thread);
		link(G_SYSCALL_GET_THREAD_ENTRY, get_thread_entry);
		link(G_SYSCALL_REGISTER_TASK_IDENTIFIER, task_id_register);
		link(G_SYSCALL_GET_TASK_FOR_IDENTIFIER, task_id_get);

		link(G_SYSCALL_MAILBOX_SEND, send_msg);
		link(G_SYSCALL_MAILBOX_RECEIVE, recv_msg);
		link(G_SYSCALL_MAILBOX_RECEIVE_WITH_IDENT, recv_topic_msg);

		link(G_SYSCALL_MESSAGE_SEND, send_message);
		link(G_SYSCALL_MESSAGE_RECEIVE, receive_message);

		link(G_SYSCALL_WAIT_FOR_IRQ, wait_for_irq);
		link(G_SYSCALL_ALLOCATE_MEMORY, alloc_mem);
		link(G_SYSCALL_SHARE_MEMORY, share_mem);
		link(G_SYSCALL_MAP_MMIO_AREA, map_mmio);
		link(G_SYSCALL_UNMAP, unmap);
		link(G_SYSCALL_SBRK, sbrk);
		link(G_SYSCALL_CALL_VM86, call_vm86);
		link(G_SYSCALL_LOWER_MEMORY_ALLOCATE, lower_malloc);
		link(G_SYSCALL_LOWER_MEMORY_FREE, lower_free);
		link(G_SYSCALL_LOG, log);
		link(G_SYSCALL_RAMDISK_FIND, ramdisk_find);
		link(G_SYSCALL_RAMDISK_FIND_CHILD, ramdisk_find_child);
		link(G_SYSCALL_RAMDISK_INFO, ramdisk_info);
		link(G_SYSCALL_RAMDISK_READ, ramdisk_read);
		link(G_SYSCALL_RAMDISK_CHILD_COUNT, ramdisk_child_count);
		link(G_SYSCALL_RAMDISK_CHILD_AT, ramdisk_child_at);
		link(G_SYSCALL_SET_VIDEO_LOG, set_video_log);
		link(G_SYSCALL_KILL, kill);
		link(G_SYSCALL_REGISTER_IRQ_HANDLER, register_irq_handler);
		link(G_SYSCALL_RESTORE_INTERRUPTED_STATE, restore_interrupted_state);
		link(G_SYSCALL_REGISTER_SIGNAL_HANDLER, register_signal_handler);
		link(G_SYSCALL_RAISE_SIGNAL, raise_signal);
		link(G_SYSCALL_CONFIGURE_PROCESS, configure_process);

		link(G_SYSCALL_CREATE_EMPTY_PROCESS, create_empty_process);
		link(G_SYSCALL_CREATE_PAGES_IN_SPACE, create_pages_in_space);
		link(G_SYSCALL_ATTACH_CREATED_PROCESS, attach_created_process);
		link(G_SYSCALL_CANCEL_PROCESS_CREATION, cancel_process_creation);
		link(G_SYSCALL_GET_CREATED_PROCESS_ID, get_created_process_id);
		link(G_SYSCALL_WRITE_TLS_MASTER_FOR_PROCESS, write_tls_master_for_process);
		link(G_SYSCALL_STORE_CLI_ARGUMENTS, cli_args_store);
		link(G_SYSCALL_RELEASE_CLI_ARGUMENTS, cli_args_release);

		link(G_SYSCALL_TEST, test);
		link(G_SYSCALL_ATOMIC_LOCK, atomic_wait);
		link(G_SYSCALL_GET_MILLISECONDS, millis);
		link(G_SYSCALL_FORK, fork);
		link(G_SYSCALL_JOIN, join);
		link(G_SYSCALL_GET_WORKING_DIRECTORY, get_working_directory);
		link(G_SYSCALL_SET_WORKING_DIRECTORY, set_working_directory);
		link(G_SYSCALL_FS_OPEN, fs_open);
		link(G_SYSCALL_FS_READ, fs_read);
		link(G_SYSCALL_FS_WRITE, fs_write);
		link(G_SYSCALL_FS_CLOSE, fs_close);
		link(G_SYSCALL_FS_STAT, fs_stat);
		link(G_SYSCALL_FS_FSTAT, fs_fstat);
		link(G_SYSCALL_FS_CLONEFD, fs_clonefd);
		link(G_SYSCALL_FS_PIPE, fs_pipe);
		link(G_SYSCALL_FS_LENGTH, fs_length);
		link(G_SYSCALL_FS_SEEK, fs_seek);
		link(G_SYSCALL_FS_TELL, fs_tell);
		link(G_SYSCALL_FS_OPEN_DIRECTORY, fs_open_directory);
		link(G_SYSCALL_FS_READ_DIRECTORY, fs_read_directory);

		link(G_SYSCALL_FS_REGISTER_AS_DELEGATE, fs_register_as_delegate);
		link(G_SYSCALL_FS_SET_TRANSACTION_STATUS, fs_set_transaction_status);
		link(G_SYSCALL_FS_CREATE_NODE, fs_create_node);
	}

	// The system call could not be handled, this might mean that the
	// process was compiled for a deprecated/messed up API library and
	// is therefore not able to run well.
	g_thread* task = g_tasking::getCurrentThread();
	g_log_debug("%! process %i tried to use non-existing syscall %i", "syscall", task->id, G_SYSCALL_CODE(state));
	task->alive = false;
	return g_tasking::schedule(state);
}
