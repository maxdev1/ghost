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
G_SYSCALL_HANDLER(get_working_directory) {

	g_syscall_fs_get_working_directory* data = (g_syscall_fs_get_working_directory*) G_SYSCALL_DATA(current_thread->cpuState);

	char* cwd = current_thread->process->workingDirectory;
	if (cwd) {
		size_t length = g_string::length(cwd);
		if (length + 1 > data->maxlen) {
			data->result = G_GET_WORKING_DIRECTORY_SIZE_EXCEEDED;
		} else {
			g_string::copy(data->buffer, cwd);
			data->result = G_GET_WORKING_DIRECTORY_SUCCESSFUL;
		}
	} else {
		data->result = G_GET_WORKING_DIRECTORY_ERROR;
	}
	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(get_executable_path) {

	g_syscall_fs_get_executable_path* data = (g_syscall_fs_get_executable_path*) G_SYSCALL_DATA(current_thread->cpuState);

	if (current_thread->process->source_path == nullptr) {
		data->buffer[0] = 0;
	} else {
		g_string::copy(data->buffer, current_thread->process->source_path);
	}

	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_register_as_delegate) {

	g_syscall_fs_register_as_delegate* data = (g_syscall_fs_register_as_delegate*) G_SYSCALL_DATA(current_thread->cpuState);
	data->result = g_filesystem::create_delegate(current_thread, data->name, data->phys_mountpoint_id, &data->mountpoint_id, &data->transaction_storage);
	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_set_transaction_status) {

	g_syscall_fs_set_transaction_status* data = (g_syscall_fs_set_transaction_status*) G_SYSCALL_DATA(current_thread->cpuState);
	g_fs_transaction_store::set_status(data->transaction, data->status);
	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_create_node) {

	g_syscall_fs_create_node* data = (g_syscall_fs_create_node*) G_SYSCALL_DATA(current_thread->cpuState);

	// check for parent
	g_fs_node* parent = g_filesystem::get_node_by_id(data->parent_id);
	if (parent == nullptr) {
		data->result = G_FS_CREATE_NODE_STATUS_FAILED_NO_PARENT;
		return current_thread;
	}

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

	// set node info
	node->type = data->type;
	node->phys_fs_id = data->phys_fs_id;

	// return node id
	data->created_id = node->id;
	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(set_working_directory) {

	g_syscall_fs_set_working_directory* data = (g_syscall_fs_set_working_directory*) G_SYSCALL_DATA(current_thread->cpuState);

	// find the target thread
	g_thread* target = nullptr;
	if (data->process == 0) {
		target = current_thread; // any task can do this for himself

	} else if (current_thread->process->securityLevel <= G_SECURITY_LEVEL_KERNEL) {
		target = (g_thread*) data->process; // only kernel-level tasks can do this for other tasks

	} else {
		g_log_warn("only kernel level tasks are allowed to set the working directory of another process");
		data->result = G_SET_WORKING_DIRECTORY_ERROR;
		return current_thread;
	}

	// get the absolute path to the new working directory
	g_local<char> absolute_path(new char[G_PATH_MAX]);
	g_filesystem::concat_as_absolute_path(target->process->workingDirectory, data->path, absolute_path());

	// if the executor sets the working directory for another thread (that is not yet attached),
	// we must supply this thread as the "unspawned target" to the transaction handler.
	g_thread* unspawned_target = ((target == current_thread) ? 0 : target);

	// perform discovery, perform setting of working directory once finished
	g_contextual<g_syscall_fs_set_working_directory*> bound_data(data, current_thread->process->pageDirectory);
	g_fs_transaction_handler_discovery_set_cwd* handler = new g_fs_transaction_handler_discovery_set_cwd(absolute_path(), bound_data, unspawned_target);
	auto start_status = handler->start_transaction(current_thread);

	if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
		return g_tasking::schedule();

	} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
		return current_thread;

	} else {
		g_log_warn("starting read transaction failed with status (%i)", start_status);
		data->result = G_SET_WORKING_DIRECTORY_ERROR;
		return current_thread;
	}
}

/**
 * Processes a file open request.
 */
G_SYSCALL_HANDLER(fs_open) {

	g_syscall_fs_open* data = (g_syscall_fs_open*) G_SYSCALL_DATA(current_thread->cpuState);

	// create an absolute path from the given path
	g_local<char> target_path(new char[G_PATH_MAX]);
	g_filesystem::concat_as_absolute_path(current_thread->process->workingDirectory, data->path, target_path());

	// create the handler that works after the node was discovered
	g_contextual<g_syscall_fs_open*> bound_data(data, current_thread->process->pageDirectory);
	g_fs_transaction_handler_discovery_open* handler = new g_fs_transaction_handler_discovery_open(target_path(), bound_data);
	auto start_status = handler->start_transaction(current_thread);

	if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
		return g_tasking::schedule();

	} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
		return current_thread;

	} else {
		g_log_warn("starting open transaction failed with status (%i)", start_status);
		data->status = G_FS_OPEN_ERROR;
		return current_thread;
	}
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_read) {

	g_syscall_fs_read* data = (g_syscall_fs_read*) G_SYSCALL_DATA(current_thread->cpuState);

	// find the filesystem node
	g_fs_node* node;
	g_file_descriptor_content* fd;
	if (!g_filesystem::node_for_descriptor(current_thread->process->main->id, data->fd, &node, &fd)) {
		data->status = G_FS_READ_INVALID_FD;
		return current_thread;
	}

	// create and start the handler
	g_contextual<g_syscall_fs_read*> bound_data(data, current_thread->process->pageDirectory);
	g_fs_transaction_handler_read* handler = new g_fs_transaction_handler_read(node, fd, bound_data);
	auto start_status = handler->start_transaction(current_thread);

	// check what happened when starting
	if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
		return g_tasking::schedule();

	} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
		return current_thread;

	} else {
		g_log_warn("starting read transaction failed with status (%i)", start_status);
		data->status = G_FS_READ_ERROR;
		return current_thread;
	}
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_write) {

	g_syscall_fs_write* data = (g_syscall_fs_write*) G_SYSCALL_DATA(current_thread->cpuState);

	// find the filesystem node
	g_fs_node* node;
	g_file_descriptor_content* fd;
	if (!g_filesystem::node_for_descriptor(current_thread->process->main->id, data->fd, &node, &fd)) {
		data->status = G_FS_WRITE_INVALID_FD;
		return current_thread;
	}

	// create and start the handler
	g_contextual<g_syscall_fs_write*> bound_data(data, current_thread->process->pageDirectory);
	g_fs_transaction_handler_write* handler = new g_fs_transaction_handler_write(node, fd, bound_data);
	auto start_status = handler->start_transaction(current_thread);

	// check what happened when starting
	if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
		return g_tasking::schedule();

	} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
		return current_thread;

	} else {
		g_log_warn("%! starting write transaction failed with status (%i)", "filesystem", start_status);
		data->status = G_FS_WRITE_ERROR;
		return current_thread;
	}
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_close) {

	g_syscall_fs_close* data = (g_syscall_fs_close*) G_SYSCALL_DATA(current_thread->cpuState);

	// find the filesystem node
	g_fs_node* node;
	g_file_descriptor_content* fd;
	if (!g_filesystem::node_for_descriptor(current_thread->process->main->id, data->fd, &node, &fd)) {
		data->status = G_FS_CLOSE_INVALID_FD;
		return current_thread;
	}

	// create and start handler
	g_contextual<g_syscall_fs_close*> bound_data(data, current_thread->process->pageDirectory);
	g_fs_transaction_handler_close* handler = new g_fs_transaction_handler_close(bound_data, fd, node);
	auto start_status = handler->start_transaction(current_thread);

	if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
		return g_tasking::schedule();

	} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
		return current_thread;

	} else {
		g_log_warn("starting close transaction failed with status (%i)", start_status);
		data->status = G_FS_CLOSE_ERROR;
		return current_thread;
	}
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_seek) {

	g_syscall_fs_seek* data = (g_syscall_fs_seek*) G_SYSCALL_DATA(current_thread->cpuState);

	// find the node
	g_fs_node* node;
	g_file_descriptor_content* fd;
	if (!g_filesystem::node_for_descriptor(current_thread->process->main->id, data->fd, &node, &fd)) {
		data->status = G_FS_SEEK_INVALID_FD;
		return current_thread;
	}

	// create and start handler
	g_contextual<g_syscall_fs_seek*> bound_data(data, current_thread->process->pageDirectory);
	g_fs_transaction_handler_get_length_seek* handler = new g_fs_transaction_handler_get_length_seek(fd, node, bound_data);
	auto start_status = handler->start_transaction(current_thread);

	if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
		return g_tasking::schedule();

	} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
		return current_thread;

	} else {
		g_log_warn("starting get-length transaction for seek failed with status (%i)", start_status);
		data->status = G_FS_SEEK_ERROR;
		return current_thread;
	}
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_length) {

	g_syscall_fs_length* data = (g_syscall_fs_length*) G_SYSCALL_DATA(current_thread->cpuState);

	bool by_fd = (data->mode & G_SYSCALL_FS_LENGTH_MODE_BY_MASK) == G_SYSCALL_FS_LENGTH_BY_FD;
	bool follow_symlinks = (data->mode & G_SYSCALL_FS_LENGTH_MODE_SYMLINK_MASK) == G_SYSCALL_FS_LENGTH_FOLLOW_SYMLINKS;

	if (by_fd) {
		// find the node
		g_fs_node* node;
		g_file_descriptor_content* fd;
		if (!g_filesystem::node_for_descriptor(current_thread->process->main->id, data->fd, &node, &fd)) {
			data->status = G_FS_LENGTH_INVALID_FD;
			return current_thread;
		}

		// create and start the handler
		g_contextual<g_syscall_fs_length*> bound_data(data, current_thread->process->pageDirectory);
		g_fs_transaction_handler_get_length_default* handler = new g_fs_transaction_handler_get_length_default(bound_data, node);
		auto start_status = handler->start_transaction(current_thread);

		if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
			return g_tasking::schedule();

		} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
			return current_thread;

		} else {
			g_log_warn("starting get-length transaction failed with status (%i)", start_status);
			data->status = G_FS_LENGTH_ERROR;
			return current_thread;
		}

	} else {
		// get absolute path for the requested path, relative to process working directory
		g_local<char> absolute_path(new char[G_PATH_MAX]);
		g_filesystem::concat_as_absolute_path(current_thread->process->workingDirectory, data->path, absolute_path());

		// create and start handler
		g_contextual<g_syscall_fs_length*> bound_data(data, current_thread->process->pageDirectory);
		g_fs_transaction_handler_discovery_get_length* handler = new g_fs_transaction_handler_discovery_get_length(absolute_path(), follow_symlinks,
				bound_data);
		auto start_status = handler->start_transaction(current_thread);

		if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
			return g_tasking::schedule();

		} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
			return current_thread;

		} else {
			g_log_warn("starting get-length transaction failed with status (%i)", start_status);
			data->status = G_FS_LENGTH_ERROR;
			return current_thread;
		}
	}

	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_tell) {

	g_syscall_fs_tell* data = (g_syscall_fs_tell*) G_SYSCALL_DATA(current_thread->cpuState);

	g_fs_node* node;
	g_file_descriptor_content* fd;

	if (g_filesystem::node_for_descriptor(current_thread->process->main->id, data->fd, &node, &fd)) {
		data->status = G_FS_TELL_SUCCESSFUL;
		data->result = fd->offset;
	} else {
		data->status = G_FS_TELL_INVALID_FD;
		data->result = -1;
	}
	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_clonefd) {

	g_syscall_fs_clonefd* data = (g_syscall_fs_clonefd*) G_SYSCALL_DATA(current_thread->cpuState);
	data->result = g_filesystem::clonefd(data->source_fd, data->source_pid, data->target_fd, data->target_pid, &data->status);

	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_pipe) {

	g_syscall_fs_pipe* data = (g_syscall_fs_pipe*) G_SYSCALL_DATA(current_thread->cpuState);
	data->status = g_filesystem::pipe(current_thread, data->blocking, &data->write_fd, &data->read_fd);
	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_open_directory) {

	g_syscall_fs_open_directory* data = (g_syscall_fs_open_directory*) G_SYSCALL_DATA(current_thread->cpuState);

	// get absolute path for the requested path, relative to process working directory
	g_local<char> absolute_path(new char[G_PATH_MAX]);
	g_filesystem::concat_as_absolute_path(current_thread->process->workingDirectory, data->path, absolute_path());

	// create handler
	g_contextual<g_syscall_fs_open_directory*> bound_data(data, current_thread->process->pageDirectory);
	g_fs_transaction_handler_discovery_open_directory* handler = new g_fs_transaction_handler_discovery_open_directory(absolute_path(), bound_data);
	handler->start_transaction(current_thread);
	return g_tasking::schedule();
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_read_directory) {

	g_syscall_fs_read_directory* data = (g_syscall_fs_read_directory*) G_SYSCALL_DATA(current_thread->cpuState);

	// create handler
	g_contextual<g_syscall_fs_read_directory*> bound_data(data, current_thread->process->pageDirectory);

	// find the folder to operate on
	auto folder = g_filesystem::get_node_by_id(data->iterator->node_id);
	if (folder == nullptr) {
		data->status = G_FS_READ_DIRECTORY_ERROR;
		return current_thread;
	}

	// handler that actually puts the next node into the iterator
	g_fs_transaction_handler_read_directory* read_handler = new g_fs_transaction_handler_read_directory(folder, bound_data);

	// check if the directory contents are complete and valid, if they are finish immediately
	if (folder->contents_valid) {
		read_handler->finish_transaction(current_thread, 0);
		delete read_handler;
		return current_thread;
	}

	// schedule a refresh
	g_fs_transaction_handler_directory_refresh* refresh_handler = new g_fs_transaction_handler_directory_refresh(folder, bound_data, read_handler);
	read_handler->causing_handler = refresh_handler;
	auto start_status = refresh_handler->start_transaction(current_thread);

	if (start_status == G_FS_TRANSACTION_START_WITH_WAITER) {
		return g_tasking::schedule();

	} else if (start_status == G_FS_TRANSACTION_START_IMMEDIATE_FINISH) {
		return current_thread;

	} else {
		g_log_warn("starting read-directory transaction failed with status (%i)", start_status);
		data->status = G_FS_READ_DIRECTORY_ERROR;
		return current_thread;
	}
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_stat) {

	g_syscall_fs_stat* data = (g_syscall_fs_stat*) G_SYSCALL_DATA(current_thread->cpuState);

	data->result = -1;

	return current_thread;
}

/**
 *
 */
G_SYSCALL_HANDLER(fs_fstat) {

	g_syscall_fs_fstat* data = (g_syscall_fs_fstat*) G_SYSCALL_DATA(current_thread->cpuState);

	data->result = -1;

	return current_thread;
}
