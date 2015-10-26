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
#define G_SYSCALL_HANDLER(handlerName)		g_thread* g_syscall_handler::handlerName(g_thread* current_thread)

/**
 *
 */
class g_syscall_handler {
public:
	static g_thread* handle(g_thread* state);

private:
	static g_thread* yield(g_thread* state);
	static g_thread* exit(g_thread* state);
	static g_thread* sleep(g_thread* state);
	static g_thread* get_pid(g_thread* state);
	static g_thread* get_tid(g_thread* state);
	static g_thread* get_pid_for_tid(g_thread* state);
	static g_thread* ramdisk_spawn(g_thread* state);
	static g_thread* create_thread(g_thread* state);
	static g_thread* get_thread_entry(g_thread* state);
	static g_thread* wait_for_irq(g_thread* state);
	static g_thread* call_vm86(g_thread* state);
	static g_thread* cli_args_store(g_thread* state);
	static g_thread* cli_args_release(g_thread* state);
	static g_thread* millis(g_thread* state);
	static g_thread* kill(g_thread* state);
	static g_thread* register_irq_handler(g_thread* state);
	static g_thread* restore_interrupted_state(g_thread* state);
	static g_thread* register_signal_handler(g_thread* state);
	static g_thread* raise_signal(g_thread* state);

	static g_thread* task_id_register(g_thread* state);
	static g_thread* task_id_get(g_thread* state);
	static g_thread* fork(g_thread* state);
	static g_thread* join(g_thread* state);
	static g_thread* atomic_wait(g_thread* state);

	static g_thread* create_empty_process(g_thread* state);
	static g_thread* create_pages_in_space(g_thread* state);
	static g_thread* attach_created_process(g_thread* state);
	static g_thread* cancel_process_creation(g_thread* state);
	static g_thread* get_created_process_id(g_thread* state);
	static g_thread* write_tls_master_for_process(g_thread* state);
	static g_thread* configure_process(g_thread* state);

	static g_thread* send_msg(g_thread* state);
	static g_thread* recv_msg(g_thread* state);
	static g_thread* recv_topic_msg(g_thread* state);
	static g_thread* send_message(g_thread* state);
	static g_thread* receive_message(g_thread* state);

	static g_thread* alloc_mem(g_thread* state);
	static g_thread* share_mem(g_thread* state);
	static g_thread* map_mmio(g_thread* state);
	static g_thread* sbrk(g_thread* state);
	static g_thread* lower_malloc(g_thread* state);
	static g_thread* lower_free(g_thread* state);
	static g_thread* unmap(g_thread* state);

	static g_thread* log(g_thread* state);
	static g_thread* set_video_log(g_thread* state);
	static g_thread* test(g_thread* state);

	static g_thread* ramdisk_find(g_thread* state);
	static g_thread* ramdisk_find_child(g_thread* state);
	static g_thread* ramdisk_info(g_thread* state);
	static g_thread* ramdisk_read(g_thread* state);
	static g_thread* ramdisk_child_count(g_thread* state);
	static g_thread* ramdisk_child_at(g_thread* state);

	static g_thread* fs_open(g_thread* state);
	static g_thread* fs_read(g_thread* state);
	static g_thread* fs_write(g_thread* state);
	static g_thread* fs_close(g_thread* state);
	static g_thread* fs_seek(g_thread* state);
	static g_thread* fs_length(g_thread* state);

	static g_thread* fs_stat(g_thread* state);
	static g_thread* fs_fstat(g_thread* state);
	static g_thread* fs_clonefd(g_thread* state);
	static g_thread* fs_pipe(g_thread* state);
	static g_thread* fs_tell(g_thread* state);
	static g_thread* fs_open_directory(g_thread* state);
	static g_thread* fs_read_directory(g_thread* state);

	static g_thread* get_working_directory(g_thread* state);
	static g_thread* set_working_directory(g_thread* state);

	static g_thread* fs_register_as_delegate(g_thread* state);
	static g_thread* fs_set_transaction_status(g_thread* state);
	static g_thread* fs_create_node(g_thread* state);

};

#endif
