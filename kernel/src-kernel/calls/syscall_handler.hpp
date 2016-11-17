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

#ifndef GHOST_SYSTEM_CALLS
#define GHOST_SYSTEM_CALLS

#include <utils/string.hpp>
#include <memory/paging.hpp>
#include <memory/memory.hpp>
#include <system/processor_state.hpp>
#include <tasking/thread.hpp>

// Macros for call code and data
#define G_SYSCALL_CODE(state)				state->eax
#define G_SYSCALL_DATA(state)				state->ebx

// Call handler macro
#define G_SYSCALL_HANDLER(handlerName)		g_thread* g_syscall_handler_##handlerName(g_thread* current_thread)

g_thread* g_syscall_handle(g_thread* state);

g_thread* g_syscall_handler_yield(g_thread* state);
g_thread* g_syscall_handler_exit(g_thread* state);
g_thread* g_syscall_handler_exit_thread(g_thread* state);
g_thread* g_syscall_handler_sleep(g_thread* state);
g_thread* g_syscall_handler_get_pid(g_thread* state);
g_thread* g_syscall_handler_get_tid(g_thread* state);
g_thread* g_syscall_handler_get_pid_for_tid(g_thread* state);
g_thread* g_syscall_handler_get_parent_pid(g_thread* state);
g_thread* g_syscall_handler_ramdisk_spawn(g_thread* state);
g_thread* g_syscall_handler_create_thread(g_thread* state);
g_thread* g_syscall_handler_get_thread_entry(g_thread* state);
g_thread* g_syscall_handler_wait_for_irq(g_thread* state);
g_thread* g_syscall_handler_call_vm86(g_thread* state);
g_thread* g_syscall_handler_cli_args_store(g_thread* state);
g_thread* g_syscall_handler_cli_args_release(g_thread* state);
g_thread* g_syscall_handler_millis(g_thread* state);
g_thread* g_syscall_handler_kill(g_thread* state);
g_thread* g_syscall_handler_register_irq_handler(g_thread* state);
g_thread* g_syscall_handler_restore_interrupted_state(g_thread* state);
g_thread* g_syscall_handler_register_signal_handler(g_thread* state);
g_thread* g_syscall_handler_raise_signal(g_thread* state);

g_thread* g_syscall_handler_task_id_register(g_thread* state);
g_thread* g_syscall_handler_task_id_get(g_thread* state);
g_thread* g_syscall_handler_fork(g_thread* state);
g_thread* g_syscall_handler_join(g_thread* state);
g_thread* g_syscall_handler_atomic_wait(g_thread* state);

g_thread* g_syscall_handler_create_empty_process(g_thread* state);
g_thread* g_syscall_handler_create_pages_in_space(g_thread* state);
g_thread* g_syscall_handler_attach_created_process(g_thread* state);
g_thread* g_syscall_handler_cancel_process_creation(g_thread* state);
g_thread* g_syscall_handler_get_created_process_id(g_thread* state);
g_thread* g_syscall_handler_write_tls_master_for_process(g_thread* state);
g_thread* g_syscall_handler_configure_process(g_thread* state);

g_thread* g_syscall_handler_send_message(g_thread* state);
g_thread* g_syscall_handler_receive_message(g_thread* state);

g_thread* g_syscall_handler_alloc_mem(g_thread* state);
g_thread* g_syscall_handler_share_mem(g_thread* state);
g_thread* g_syscall_handler_map_mmio(g_thread* state);
g_thread* g_syscall_handler_sbrk(g_thread* state);
g_thread* g_syscall_handler_lower_malloc(g_thread* state);
g_thread* g_syscall_handler_lower_free(g_thread* state);
g_thread* g_syscall_handler_unmap(g_thread* state);

g_thread* g_syscall_handler_log(g_thread* state);
g_thread* g_syscall_handler_set_video_log(g_thread* state);
g_thread* g_syscall_handler_test(g_thread* state);
g_thread* g_syscall_handler_kernquery(g_thread* state);

g_thread* g_syscall_handler_ramdisk_find(g_thread* state);
g_thread* g_syscall_handler_ramdisk_find_child(g_thread* state);
g_thread* g_syscall_handler_ramdisk_info(g_thread* state);
g_thread* g_syscall_handler_ramdisk_read(g_thread* state);
g_thread* g_syscall_handler_ramdisk_child_count(g_thread* state);
g_thread* g_syscall_handler_ramdisk_child_at(g_thread* state);

g_thread* g_syscall_handler_fs_open(g_thread* state);
g_thread* g_syscall_handler_fs_read(g_thread* state);
g_thread* g_syscall_handler_fs_write(g_thread* state);
g_thread* g_syscall_handler_fs_close(g_thread* state);
g_thread* g_syscall_handler_fs_seek(g_thread* state);
g_thread* g_syscall_handler_fs_length(g_thread* state);

g_thread* g_syscall_handler_fs_stat(g_thread* state);
g_thread* g_syscall_handler_fs_fstat(g_thread* state);
g_thread* g_syscall_handler_fs_clonefd(g_thread* state);
g_thread* g_syscall_handler_fs_pipe(g_thread* state);
g_thread* g_syscall_handler_fs_tell(g_thread* state);
g_thread* g_syscall_handler_fs_open_directory(g_thread* state);
g_thread* g_syscall_handler_fs_read_directory(g_thread* state);

g_thread* g_syscall_handler_get_executable_path(g_thread* state);
g_thread* g_syscall_handler_get_working_directory(g_thread* state);
g_thread* g_syscall_handler_set_working_directory(g_thread* state);

g_thread* g_syscall_handler_fs_register_as_delegate(g_thread* state);
g_thread* g_syscall_handler_fs_set_transaction_status(g_thread* state);
g_thread* g_syscall_handler_fs_create_node(g_thread* state);

#endif
