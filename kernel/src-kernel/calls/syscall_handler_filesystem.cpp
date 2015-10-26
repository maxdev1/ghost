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

#include <debug/debug_interface_kernel.hpp>
#include "calls/syscall_handler.hpp"

#include "filesystem/filesystem.hpp"
#include "filesystem/fs_transaction_handler_discovery_set_cwd.hpp"
#include "filesystem/fs_transaction_handler_discovery_open.hpp"
#include "filesystem/fs_transaction_handler_discovery_open_directory.hpp"
#include "filesystem/fs_transaction_handler_get_length_seek.hpp"
#include "filesystem/fs_transaction_handler_get_length_default.hpp"
#include "filesystem/fs_transaction_handler_discovery_get_length.hpp"

#include "ghost/utils/local.hpp"
#include "tasking/tasking.hpp"
#include "utils/string.hpp"
#include "memory/contextual.hpp"
#include "logger/logger.hpp"

/**
 *
 */
G_SYSCALL_HANDLER(set_working_directory) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_set_working_directory* data = (g_syscall_fs_set_working_directory*) G_SYSCALL_DATA(state);

	g_thread* target = nullptr;
	if (data->process == 0) {
		target = task;

	} else if (task->process->securityLevel <= G_SECURITY_LEVEL_KERNEL) {
		// only kernel level task (spawner for example) is allowed to do this for other processes
		target = (g_thread*) data->process;

	}

	if (target != nullptr) {
		// get the absolute path to the new working directory
		g_local<char> workdir_abs(new char[G_PATH_MAX]);
		g_filesystem::concat_as_absolute_path(target->process->workingDirectory, data->path, workdir_abs());

		// if the executor sets the working directory for another thread (that is not yet attached),
		// we must supply this thread as the "unspawned target" to the transaction handler.
		g_thread* unspawned_target = (target == task) ? 0 : target;

		// perform discovery, perform setting of working directory once finished
		g_contextual<g_syscall_fs_set_working_directory*> bound_data(data, task->process->pageDirectory);
		g_fs_transaction_handler_discovery_set_cwd* handler = new g_fs_transaction_handler_discovery_set_cwd(workdir_abs(), bound_data, unspawned_target);
		g_filesystem::discover_absolute_path(task, workdir_abs(), handler);
		return g_tasking::switchTask(state);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(get_working_directory) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_get_working_directory* data = (g_syscall_fs_get_working_directory*) G_SYSCALL_DATA(state);

	g_string::copy(data->buffer, task->process->workingDirectory);

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_register_as_delegate) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_register_as_delegate* data = (g_syscall_fs_register_as_delegate*) G_SYSCALL_DATA(state);
	data->result = g_filesystem::create_delegate(task, data->name, data->phys_mountpoint_id, &data->mountpoint_id, &data->transaction_storage);
	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_set_transaction_status) {

	g_syscall_fs_set_transaction_status* data = (g_syscall_fs_set_transaction_status*) G_SYSCALL_DATA(state);
	g_fs_transaction_store::set_status(data->transaction, data->status);
	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_create_node) {

	g_syscall_fs_create_node* data = (g_syscall_fs_create_node*) G_SYSCALL_DATA(state);

	// check for parent
	g_fs_node* parent = g_filesystem::get_node_by_id(data->parent_id);

	if (parent == 0) {
		data->result = G_FS_CREATE_NODE_STATUS_FAILED_NO_PARENT;

	} else {
		// check for already existing child
		g_fs_node* node = parent->find_child(data->name);
		if (node) {
			data->result = G_FS_CREATE_NODE_STATUS_UPDATED;

		} else {
			// create the node
			node = g_filesystem::create_node();
			node->name = new char[g_string::length(data->name) + 1];
			g_string::copy(node->name, data->name);
			parent->add_child(node);
			G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(node);

			data->result = G_FS_CREATE_NODE_STATUS_CREATED;
		}

		// fill info in node
		node->type = data->type;
		node->phys_fs_id = data->phys_fs_id;
		data->created_id = node->id;
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_open) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_open* data = (g_syscall_fs_open*) G_SYSCALL_DATA(state);

	// get absolute path for the requested path, relative to process working directory
	g_local<char> absolute_path(new char[G_PATH_MAX]);
	g_filesystem::concat_as_absolute_path(task->process->workingDirectory, data->path, absolute_path());

	// create handler
	g_contextual<g_syscall_fs_open*> bound_data(data, task->process->pageDirectory);
	g_fs_transaction_handler_discovery_open* handler = new g_fs_transaction_handler_discovery_open(absolute_path(), bound_data);

	// discover path and let go
	g_filesystem::discover_absolute_path(task, absolute_path(), handler);
	return g_tasking::switchTask(state);
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_read) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_read* data = (g_syscall_fs_read*) G_SYSCALL_DATA(state);

	// Find the file descriptor and the matching virtual file system node first:
	g_fs_node* node;
	g_file_descriptor_content* fd;
	if (g_filesystem::node_for_descriptor(task->process->main->id, data->fd, &node, &fd)) {

		/**
		 * Create handler for the transaction. The transaction handler is then asked to
		 * start the transaction. After that, the transaction status is immediately checked
		 * as it might instantly be finished.
		 */
		g_contextual<g_syscall_fs_read*> bound_data(data, task->process->pageDirectory);
		g_fs_transaction_handler_read* handler = new g_fs_transaction_handler_read(node, fd, bound_data);
		g_fs_transaction_handler_start_status start_status = handler->start_transaction(task);

		/**
		 * Depending on what the start of the transaction resulted in, we must either let the
		 * requesting task wait (a waiter was appended) or let it continue immediately.
		 */
		if (start_status == G_FS_TRANSACTION_STARTED_WITH_WAITER) {
			return g_tasking::switchTask(state);

		} else if (start_status == G_FS_TRANSACTION_STARTED_AND_FINISHED) {
			return state;

		} else {
			g_log_warn("starting read transaction failed with status (%i)", start_status);
		}
	}

	data->status = G_FS_READ_INVALID_FD;
	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_write) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_write* data = (g_syscall_fs_write*) G_SYSCALL_DATA(state);

	// See {fs_read} for an explanation
	g_fs_node* node;
	g_file_descriptor_content* fd;
	if (g_filesystem::node_for_descriptor(task->process->main->id, data->fd, &node, &fd)) {

		g_contextual<g_syscall_fs_write*> bound_data(data, task->process->pageDirectory);
		g_fs_transaction_handler_write* handler = new g_fs_transaction_handler_write(node, fd, bound_data);
		g_fs_transaction_handler_start_status start_status = handler->start_transaction(task);

		if (start_status == G_FS_TRANSACTION_STARTED_WITH_WAITER) {
			return g_tasking::switchTask(state);

		} else if (start_status == G_FS_TRANSACTION_STARTED_AND_FINISHED) {
			return state;

		} else {
			g_log_warn("starting write transaction failed with status (%i)", start_status);
		}
	}

	data->status = G_FS_WRITE_INVALID_FD;
	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_close) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_close* data = (g_syscall_fs_close*) G_SYSCALL_DATA(state);

	g_fs_node* node;
	g_file_descriptor_content* fd;
	if (g_filesystem::node_for_descriptor(task->process->main->id, data->fd, &node, &fd)) {
		data->result = g_filesystem::close(task->process->main->id, node, fd, &data->status);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_seek) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_seek* data = (g_syscall_fs_seek*) G_SYSCALL_DATA(state);

	g_fs_node* node;
	g_file_descriptor_content* fd;
	if (g_filesystem::node_for_descriptor(task->process->main->id, data->fd, &node, &fd)) {
		g_contextual<g_syscall_fs_seek*> bound_data(data, task->process->pageDirectory);
		g_fs_transaction_handler_get_length_seek* handler = new g_fs_transaction_handler_get_length_seek(fd, bound_data);

		g_filesystem::get_length(task, node, handler);
		return g_tasking::switchTask(state);
	} else {
		data->status = G_FS_SEEK_INVALID_FD;
		return state;
	}
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_length) {

	g_thread* task = g_tasking::getCurrentThread();

	g_syscall_fs_length* data = (g_syscall_fs_length*) G_SYSCALL_DATA(state);

	bool by_fd = (data->mode & G_SYSCALL_FS_LENGTH_MODE_BY_MASK) == G_SYSCALL_FS_LENGTH_BY_FD;
	bool follow_symlinks = (data->mode & G_SYSCALL_FS_LENGTH_MODE_SYMLINK_MASK) == G_SYSCALL_FS_LENGTH_FOLLOW_SYMLINKS;

	g_contextual<g_syscall_fs_length*> bound_data(data, task->process->pageDirectory);

	if (by_fd) {
		g_fs_node* node;
		g_file_descriptor_content* fd;

		if (g_filesystem::node_for_descriptor(task->process->main->id, data->fd, &node, &fd)) {
			g_fs_transaction_handler_get_length_default* handler = new g_fs_transaction_handler_get_length_default(bound_data);
			g_filesystem::get_length(task, node, handler);
			return g_tasking::switchTask(state);
		} else {
			data->status = G_FS_LENGTH_INVALID_FD;
			return state;
		}
	} else {
		// get absolute path for the requested path, relative to process working directory
		g_local<char> absolute_path(new char[G_PATH_MAX]);
		g_filesystem::concat_as_absolute_path(task->process->workingDirectory, data->path, absolute_path());

		g_fs_transaction_handler_discovery_get_length* handler = new g_fs_transaction_handler_discovery_get_length(absolute_path(), bound_data);
		g_filesystem::discover_absolute_path(task, absolute_path(), handler, follow_symlinks);
		return g_tasking::switchTask(state);
	}

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_tell) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_tell* data = (g_syscall_fs_tell*) G_SYSCALL_DATA(state);

	g_fs_node* node;
	g_file_descriptor_content* fd;

	if (g_filesystem::node_for_descriptor(task->process->main->id, data->fd, &node, &fd)) {
		data->status = G_FS_TELL_SUCCESSFUL;
		data->result = fd->offset;
	} else {
		data->status = G_FS_TELL_INVALID_FD;
		data->result = -1;
	}
	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_clonefd) {

	g_thread* task = g_tasking::getCurrentThread();

	g_syscall_fs_clonefd* data = (g_syscall_fs_clonefd*) G_SYSCALL_DATA(state);
	data->result = g_filesystem::clonefd(data->source_fd, data->source_pid, data->target_fd, data->target_pid, &data->status);

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_pipe) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_pipe* data = (g_syscall_fs_pipe*) G_SYSCALL_DATA(state);
	data->status = g_filesystem::pipe(task, &data->write_fd, &data->read_fd);
	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_open_directory) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_open_directory* data = (g_syscall_fs_open_directory*) G_SYSCALL_DATA(state);

	// get absolute path for the requested path, relative to process working directory
	g_local<char> absolute_path(new char[G_PATH_MAX]);
	g_filesystem::concat_as_absolute_path(task->process->workingDirectory, data->path, absolute_path());

	// create handler
	g_contextual<g_syscall_fs_open_directory*> bound_data(data, task->process->pageDirectory);
	g_fs_transaction_handler_discovery_open_directory* handler = new g_fs_transaction_handler_discovery_open_directory(absolute_path(), bound_data);

	// discover path and let go
	g_filesystem::discover_absolute_path(task, absolute_path(), handler);
	return g_tasking::switchTask(state);
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_read_directory) {

	g_thread* task = g_tasking::getCurrentThread();
	g_syscall_fs_read_directory* data = (g_syscall_fs_read_directory*) G_SYSCALL_DATA(state);

	// create handler
	g_contextual<g_syscall_fs_read_directory*> bound_data(data, task->process->pageDirectory);

	g_filesystem::read_directory(task, data->iterator->node_id, data->iterator->position, bound_data);
	return g_tasking::switchTask(state);
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_stat) {

	g_thread* task = g_tasking::getCurrentThread();

	g_syscall_fs_stat* data = (g_syscall_fs_stat*) G_SYSCALL_DATA(state);
	g_filesystem::stat(task, data->path, data->follow_symlinks, &data->stats);

	return state;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_fstat) {

	g_thread* task = g_tasking::getCurrentThread();

	g_syscall_fs_fstat* data = (g_syscall_fs_fstat*) G_SYSCALL_DATA(state);
	g_filesystem::fstat(task, data->fd, &data->stats);

	return state;
}
