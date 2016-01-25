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

// Macros for call code and data
#define G_SYSCALL_CODE(state)				state->eax
#define G_SYSCALL_DATA(state)				state->ebx

// Call handler macro
#define G_SYSCALL_HANDLER(handlerName)		g_processor_state* g_syscall_handler::handlerName(g_processor_state* state)

/**
 *
 */
class g_syscall_handler {
public:
	static g_processor_state* handle(g_processor_state* state);

private:
	static g_processor_state* yield(g_processor_state* state);
	static g_processor_state* exit(g_processor_state* state);
	static g_processor_state* sleep(g_processor_state* state);
	static g_processor_state* get_pid(g_processor_state* state);
	static g_processor_state* get_tid(g_processor_state* state);
	static g_processor_state* get_pid_for_tid(g_processor_state* state);
	static g_processor_state* ramdisk_spawn(g_processor_state* state);
	static g_processor_state* create_thread(g_processor_state* state);
	static g_processor_state* get_thread_entry(g_processor_state* state);
	static g_processor_state* wait_for_irq(g_processor_state* state);
	static g_processor_state* call_vm86(g_processor_state* state);
	static g_processor_state* cli_args_store(g_processor_state* state);
	static g_processor_state* cli_args_release(g_processor_state* state);
	static g_processor_state* millis(g_processor_state* state);
	static g_processor_state* kill(g_processor_state* state);
	static g_processor_state* register_irq_handler(g_processor_state* state);
	static g_processor_state* restore_interrupted_state(g_processor_state* state);
	static g_processor_state* register_signal_handler(g_processor_state* state);
	static g_processor_state* raise_signal(g_processor_state* state);

	static g_processor_state* task_id_register(g_processor_state* state);
	static g_processor_state* task_id_get(g_processor_state* state);
	static g_processor_state* fork(g_processor_state* state);
	static g_processor_state* join(g_processor_state* state);
	static g_processor_state* atomic_wait(g_processor_state* state);

	static g_processor_state* create_empty_process(g_processor_state* state);
	static g_processor_state* create_pages_in_space(g_processor_state* state);
	static g_processor_state* attach_created_process(g_processor_state* state);
	static g_processor_state* cancel_process_creation(g_processor_state* state);
	static g_processor_state* get_created_process_id(g_processor_state* state);
	static g_processor_state* write_tls_master_for_process(g_processor_state* state);
	static g_processor_state* configure_process(g_processor_state* state);

	static g_processor_state* send_msg(g_processor_state* state);
	static g_processor_state* recv_msg(g_processor_state* state);
	static g_processor_state* recv_topic_msg(g_processor_state* state);
	static g_processor_state* send_message(g_processor_state* state);
	static g_processor_state* receive_message(g_processor_state* state);

	static g_processor_state* alloc_mem(g_processor_state* state);
	static g_processor_state* share_mem(g_processor_state* state);
	static g_processor_state* map_mmio(g_processor_state* state);
	static g_processor_state* sbrk(g_processor_state* state);
	static g_processor_state* lower_malloc(g_processor_state* state);
	static g_processor_state* lower_free(g_processor_state* state);
	static g_processor_state* unmap(g_processor_state* state);

	static g_processor_state* log(g_processor_state* state);
	static g_processor_state* set_video_log(g_processor_state* state);
	static g_processor_state* test(g_processor_state* state);

	static g_processor_state* ramdisk_find(g_processor_state* state);
	static g_processor_state* ramdisk_find_child(g_processor_state* state);
	static g_processor_state* ramdisk_info(g_processor_state* state);
	static g_processor_state* ramdisk_read(g_processor_state* state);
	static g_processor_state* ramdisk_child_count(g_processor_state* state);
	static g_processor_state* ramdisk_child_at(g_processor_state* state);

	static g_processor_state* fs_open(g_processor_state* state);
	static g_processor_state* fs_read(g_processor_state* state);
	static g_processor_state* fs_write(g_processor_state* state);
	static g_processor_state* fs_close(g_processor_state* state);
	static g_processor_state* fs_seek(g_processor_state* state);
	static g_processor_state* fs_length(g_processor_state* state);

	static g_processor_state* fs_stat(g_processor_state* state);
	static g_processor_state* fs_fstat(g_processor_state* state);
	static g_processor_state* fs_clonefd(g_processor_state* state);
	static g_processor_state* fs_pipe(g_processor_state* state);
	static g_processor_state* fs_tell(g_processor_state* state);
	static g_processor_state* fs_open_directory(g_processor_state* state);
	static g_processor_state* fs_read_directory(g_processor_state* state);

	static g_processor_state* get_working_directory(g_processor_state* state);
	static g_processor_state* set_working_directory(g_processor_state* state);

	static g_processor_state* fs_register_as_delegate(g_processor_state* state);
	static g_processor_state* fs_set_transaction_status(g_processor_state* state);
	static g_processor_state* fs_create_node(g_processor_state* state);

};

#endif
