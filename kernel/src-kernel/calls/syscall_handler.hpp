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
#include <system/cpu_state.hpp>
#include <memory/paging.hpp>
#include <memory/memory.hpp>

// Macros for call code and data
#define G_SYSCALL_CODE(state)				state->eax
#define G_SYSCALL_DATA(state)				state->ebx

// Call handler macro
#define G_SYSCALL_HANDLER(handlerName)		g_cpu_state* g_syscall_handler::handlerName(g_cpu_state* state)

/**
 *
 */
class g_syscall_handler {
public:
	static g_cpu_state* handle(g_cpu_state* state);

private:
	static g_cpu_state* yield(g_cpu_state* state);
	static g_cpu_state* exit(g_cpu_state* state);
	static g_cpu_state* sleep(g_cpu_state* state);
	static g_cpu_state* get_pid(g_cpu_state* state);
	static g_cpu_state* get_tid(g_cpu_state* state);
	static g_cpu_state* get_pid_for_tid(g_cpu_state* state);
	static g_cpu_state* ramdisk_spawn(g_cpu_state* state);
	static g_cpu_state* create_thread(g_cpu_state* state);
	static g_cpu_state* get_thread_entry(g_cpu_state* state);
	static g_cpu_state* wait_for_irq(g_cpu_state* state);
	static g_cpu_state* call_vm86(g_cpu_state* state);
	static g_cpu_state* cli_args_store(g_cpu_state* state);
	static g_cpu_state* cli_args_release(g_cpu_state* state);
	static g_cpu_state* millis(g_cpu_state* state);
	static g_cpu_state* kill(g_cpu_state* state);
	static g_cpu_state* register_irq_handler(g_cpu_state* state);
	static g_cpu_state* restore_interrupted_state(g_cpu_state* state);
	static g_cpu_state* register_signal_handler(g_cpu_state* state);
	static g_cpu_state* raise_signal(g_cpu_state* state);

	static g_cpu_state* task_id_register(g_cpu_state* state);
	static g_cpu_state* task_id_get(g_cpu_state* state);
	static g_cpu_state* fork(g_cpu_state* state);
	static g_cpu_state* join(g_cpu_state* state);
	static g_cpu_state* atomic_wait(g_cpu_state* state);

	static g_cpu_state* create_empty_process(g_cpu_state* state);
	static g_cpu_state* create_pages_in_space(g_cpu_state* state);
	static g_cpu_state* attach_created_process(g_cpu_state* state);
	static g_cpu_state* cancel_process_creation(g_cpu_state* state);
	static g_cpu_state* get_created_process_id(g_cpu_state* state);
	static g_cpu_state* write_tls_master_for_process(g_cpu_state* state);

	static g_cpu_state* send_msg(g_cpu_state* state);
	static g_cpu_state* recv_msg(g_cpu_state* state);
	static g_cpu_state* recv_topic_msg(g_cpu_state* state);
	static g_cpu_state* send_message(g_cpu_state* state);
	static g_cpu_state* receive_message(g_cpu_state* state);

	static g_cpu_state* alloc_mem(g_cpu_state* state);
	static g_cpu_state* share_mem(g_cpu_state* state);
	static g_cpu_state* map_mmio(g_cpu_state* state);
	static g_cpu_state* sbrk(g_cpu_state* state);
	static g_cpu_state* lower_malloc(g_cpu_state* state);
	static g_cpu_state* lower_free(g_cpu_state* state);
	static g_cpu_state* unmap(g_cpu_state* state);

	static g_cpu_state* log(g_cpu_state* state);
	static g_cpu_state* set_video_log(g_cpu_state* state);
	static g_cpu_state* test(g_cpu_state* state);

	static g_cpu_state* ramdisk_find(g_cpu_state* state);
	static g_cpu_state* ramdisk_find_child(g_cpu_state* state);
	static g_cpu_state* ramdisk_info(g_cpu_state* state);
	static g_cpu_state* ramdisk_read(g_cpu_state* state);
	static g_cpu_state* ramdisk_child_count(g_cpu_state* state);
	static g_cpu_state* ramdisk_child_at(g_cpu_state* state);

	static g_cpu_state* fs_open(g_cpu_state* state);
	static g_cpu_state* fs_read(g_cpu_state* state);
	static g_cpu_state* fs_write(g_cpu_state* state);
	static g_cpu_state* fs_close(g_cpu_state* state);
	static g_cpu_state* fs_seek(g_cpu_state* state);
	static g_cpu_state* fs_length(g_cpu_state* state);

	static g_cpu_state* fs_stat(g_cpu_state* state);
	static g_cpu_state* fs_fstat(g_cpu_state* state);
	static g_cpu_state* fs_clonefd(g_cpu_state* state);
	static g_cpu_state* fs_pipe(g_cpu_state* state);
	static g_cpu_state* fs_tell(g_cpu_state* state);
	static g_cpu_state* fs_open_directory(g_cpu_state* state);
	static g_cpu_state* fs_read_directory(g_cpu_state* state);

	static g_cpu_state* get_working_directory(g_cpu_state* state);
	static g_cpu_state* set_working_directory(g_cpu_state* state);

	static g_cpu_state* fs_register_as_delegate(g_cpu_state* state);
	static g_cpu_state* fs_set_transaction_status(g_cpu_state* state);
	static g_cpu_state* fs_create_node(g_cpu_state* state);

};

#endif
