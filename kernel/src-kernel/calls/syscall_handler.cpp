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
#include <memory/physical/pp_allocator.hpp>
#include <memory/address_space.hpp>
#include <memory/temporary_paging_util.hpp>
#include <memory/constants.hpp>
#include <memory/lower_heap.hpp>
#include <system/interrupts/handling/interrupt_request_dispatcher.hpp>

#define SYSCALL_LINK(val, func)															\
	case val: return g_syscall_handler_##func(current_thread);

/**
 * The system call handler is called by the RequestHandler when a system call is executed.
 * The handler then checks which call was requested by taking it's number from
 * the callers EAX and redirects to the specific system call function.
 */
g_thread* g_syscall_handle(g_thread* current_thread) {

	uint32_t call = G_SYSCALL_CODE(current_thread->cpuState);

	switch (call) {
		SYSCALL_LINK(G_SYSCALL_YIELD, yield);
		SYSCALL_LINK(G_SYSCALL_EXIT, exit);
		SYSCALL_LINK(G_SYSCALL_EXIT_THREAD, exit_thread);
		SYSCALL_LINK(G_SYSCALL_GET_PROCESS_ID, get_pid);
		SYSCALL_LINK(G_SYSCALL_GET_TASK_ID, get_tid);
		SYSCALL_LINK(G_SYSCALL_GET_PROCESS_ID_FOR_TASK_ID, get_pid_for_tid);
		SYSCALL_LINK(G_SYSCALL_GET_PARENT_PROCESS_ID, get_parent_pid);
		SYSCALL_LINK(G_SYSCALL_RAMDISK_SPAWN, ramdisk_spawn);
		SYSCALL_LINK(G_SYSCALL_SLEEP, sleep);
		SYSCALL_LINK(G_SYSCALL_CREATE_THREAD, create_thread);
		SYSCALL_LINK(G_SYSCALL_GET_THREAD_ENTRY, get_thread_entry);
		SYSCALL_LINK(G_SYSCALL_REGISTER_TASK_IDENTIFIER, task_id_register);
		SYSCALL_LINK(G_SYSCALL_GET_TASK_FOR_IDENTIFIER, task_id_get);

		SYSCALL_LINK(G_SYSCALL_MESSAGE_SEND, send_message);
		SYSCALL_LINK(G_SYSCALL_MESSAGE_RECEIVE, receive_message);

		SYSCALL_LINK(G_SYSCALL_WAIT_FOR_IRQ, wait_for_irq);
		SYSCALL_LINK(G_SYSCALL_ALLOCATE_MEMORY, alloc_mem);
		SYSCALL_LINK(G_SYSCALL_SHARE_MEMORY, share_mem);
		SYSCALL_LINK(G_SYSCALL_MAP_MMIO_AREA, map_mmio);
		SYSCALL_LINK(G_SYSCALL_UNMAP, unmap);
		SYSCALL_LINK(G_SYSCALL_SBRK, sbrk);
		SYSCALL_LINK(G_SYSCALL_CALL_VM86, call_vm86);
		SYSCALL_LINK(G_SYSCALL_LOWER_MEMORY_ALLOCATE, lower_malloc);
		SYSCALL_LINK(G_SYSCALL_LOWER_MEMORY_FREE, lower_free);
		SYSCALL_LINK(G_SYSCALL_LOG, log);
		SYSCALL_LINK(G_SYSCALL_RAMDISK_FIND, ramdisk_find);
		SYSCALL_LINK(G_SYSCALL_RAMDISK_FIND_CHILD, ramdisk_find_child);
		SYSCALL_LINK(G_SYSCALL_RAMDISK_INFO, ramdisk_info);
		SYSCALL_LINK(G_SYSCALL_RAMDISK_READ, ramdisk_read);
		SYSCALL_LINK(G_SYSCALL_RAMDISK_CHILD_COUNT, ramdisk_child_count);
		SYSCALL_LINK(G_SYSCALL_RAMDISK_CHILD_AT, ramdisk_child_at);
		SYSCALL_LINK(G_SYSCALL_SET_VIDEO_LOG, set_video_log);
		SYSCALL_LINK(G_SYSCALL_KILL, kill);
		SYSCALL_LINK(G_SYSCALL_REGISTER_IRQ_HANDLER, register_irq_handler);
		SYSCALL_LINK(G_SYSCALL_RESTORE_INTERRUPTED_STATE, restore_interrupted_state);
		SYSCALL_LINK(G_SYSCALL_REGISTER_SIGNAL_HANDLER, register_signal_handler);
		SYSCALL_LINK(G_SYSCALL_RAISE_SIGNAL, raise_signal);
		SYSCALL_LINK(G_SYSCALL_CONFIGURE_PROCESS, configure_process);

		SYSCALL_LINK(G_SYSCALL_CREATE_EMPTY_PROCESS, create_empty_process);
		SYSCALL_LINK(G_SYSCALL_CREATE_PAGES_IN_SPACE, create_pages_in_space);
		SYSCALL_LINK(G_SYSCALL_ATTACH_CREATED_PROCESS, attach_created_process);
		SYSCALL_LINK(G_SYSCALL_CANCEL_PROCESS_CREATION, cancel_process_creation);
		SYSCALL_LINK(G_SYSCALL_GET_CREATED_PROCESS_ID, get_created_process_id);
		SYSCALL_LINK(G_SYSCALL_WRITE_TLS_MASTER_FOR_PROCESS, write_tls_master_for_process);
		SYSCALL_LINK(G_SYSCALL_STORE_CLI_ARGUMENTS, cli_args_store);
		SYSCALL_LINK(G_SYSCALL_RELEASE_CLI_ARGUMENTS, cli_args_release);

		SYSCALL_LINK(G_SYSCALL_TEST, test);
		SYSCALL_LINK(G_SYSCALL_KERNQUERY, kernquery);
		SYSCALL_LINK(G_SYSCALL_ATOMIC_LOCK, atomic_wait);
		SYSCALL_LINK(G_SYSCALL_GET_MILLISECONDS, millis);
		SYSCALL_LINK(G_SYSCALL_FORK, fork);
		SYSCALL_LINK(G_SYSCALL_JOIN, join);
		SYSCALL_LINK(G_SYSCALL_GET_WORKING_DIRECTORY, get_working_directory);
		SYSCALL_LINK(G_SYSCALL_GET_EXECUTABLE_PATH, get_executable_path);
		SYSCALL_LINK(G_SYSCALL_SET_WORKING_DIRECTORY, set_working_directory);
		SYSCALL_LINK(G_SYSCALL_FS_OPEN, fs_open);
		SYSCALL_LINK(G_SYSCALL_FS_READ, fs_read);
		SYSCALL_LINK(G_SYSCALL_FS_WRITE, fs_write);
		SYSCALL_LINK(G_SYSCALL_FS_CLOSE, fs_close);
		SYSCALL_LINK(G_SYSCALL_FS_STAT, fs_stat);
		SYSCALL_LINK(G_SYSCALL_FS_FSTAT, fs_fstat);
		SYSCALL_LINK(G_SYSCALL_FS_CLONEFD, fs_clonefd);
		SYSCALL_LINK(G_SYSCALL_FS_PIPE, fs_pipe);
		SYSCALL_LINK(G_SYSCALL_FS_LENGTH, fs_length);
		SYSCALL_LINK(G_SYSCALL_FS_SEEK, fs_seek);
		SYSCALL_LINK(G_SYSCALL_FS_TELL, fs_tell);
		SYSCALL_LINK(G_SYSCALL_FS_OPEN_DIRECTORY, fs_open_directory);
		SYSCALL_LINK(G_SYSCALL_FS_READ_DIRECTORY, fs_read_directory);

		SYSCALL_LINK(G_SYSCALL_FS_REGISTER_AS_DELEGATE, fs_register_as_delegate);
		SYSCALL_LINK(G_SYSCALL_FS_SET_TRANSACTION_STATUS, fs_set_transaction_status);
		SYSCALL_LINK(G_SYSCALL_FS_CREATE_NODE, fs_create_node);
	}

	// The system call could not be handled, this might mean that the
	// process was compiled for a deprecated/messed up API library and
	// is therefore not able to run well.
	g_log_debug("%! process %i tried to use non-existing syscall %i", "syscall", current_thread->id, G_SYSCALL_CODE(current_thread->cpuState));
	current_thread->alive = false;
	return g_tasking::schedule();
}
