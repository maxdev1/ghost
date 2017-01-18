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

#include "filesystem/filesystem.hpp"
#include "filesystem/fs_descriptors.hpp"
#include "filesystem/fs_transaction_store.hpp"
#include "filesystem/fs_delegate.hpp"
#include "filesystem/fs_delegate_mount.hpp"
#include "filesystem/fs_delegate_ramdisk.hpp"
#include "filesystem/fs_delegate_pipe.hpp"
#include "filesystem/fs_delegate_tasked.hpp"
#include "filesystem/pipes.hpp"

#include "logger/logger.hpp"

#include "ghost/utils/local.hpp"
#include "utils/hash_map.hpp"
#include "utils/list_entry.hpp"
#include "utils/string.hpp"

/**
 *
 */
static g_fs_virt_id node_next_id = 0;
static g_hash_map<g_fs_virt_id, g_fs_node*>* nodes;

static g_fs_node* root;
static g_fs_node* pipe_root;
static g_fs_node* mount_root;

/**
 *
 */
void g_filesystem::initialize() {
	g_pipes::initialize();
	g_file_descriptors::initialize();
	g_fs_transaction_store::initialize();
	nodes = new g_hash_map<g_fs_virt_id, g_fs_node*>();

	// create root
	root = create_node();
	root->type = G_FS_NODE_TYPE_ROOT;

	// mount root
	mount_root = create_node();
	mount_root->set_delegate(new g_fs_delegate_mount());
	mount_root->name = (char*) "mount";
	mount_root->type = G_FS_NODE_TYPE_MOUNTPOINT;
	root->add_child(mount_root);
	G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(mount_root);

	// ramdisk root
	g_fs_node* ramdisk_root = create_node();
	g_fs_delegate* ramdisk_delegate = new g_fs_delegate_ramdisk();
	ramdisk_root->set_delegate(ramdisk_delegate);
	ramdisk_root->name = (char*) "ramdisk";
	ramdisk_root->type = G_FS_NODE_TYPE_MOUNTPOINT;
	mount_root->add_child(ramdisk_root);
	G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(ramdisk_root);

	// pipe root
	pipe_root = create_node();
	pipe_root->set_delegate(new g_fs_delegate_pipe());
	pipe_root->name = (char*) "pipe";
	pipe_root->type = G_FS_NODE_TYPE_MOUNTPOINT;
	mount_root->add_child(pipe_root);
	G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(pipe_root);

	// ramdisk is root
	root->set_delegate(ramdisk_delegate);

	g_log_info("%! initial resources created", "filesystem");
}

/**
 *
 */
g_fs_node* g_filesystem::get_root() {
	return root;
}

/**
 *
 */
g_fs_node* g_filesystem::get_node_by_id(g_fs_virt_id id) {
	auto entry = nodes->get(id);
	if (entry) {
		return entry->value;
	}
	return 0;
}

/**
 *
 */
g_fs_node* g_filesystem::create_node() {

	g_fs_node* node = new g_fs_node();
	node->id = node_next_id++;
	nodes->put(node->id, node);
	return node;
}

/**
 *
 */
void g_filesystem::find_existing(char* absolute_path, g_fs_node** out_parent, g_fs_node** out_child, char* name_current, bool follow_symlinks) {

	g_fs_node* parent = 0;
	g_fs_node* child = root;

	char* abspos = absolute_path;

	while (true) {
		// parent is now child
		parent = child;

		// copy first name on left into current
		while (*abspos == '/') {
			++abspos;
		}

		char* curpos = name_current;
		int curlen = 0;
		while (*abspos != 0 && *abspos != '/') {
			*curpos++ = *abspos++;
			++curlen;
		}
		*curpos = 0;

		// quit if nothing left in path
		if (curlen == 0) {
			break;
		}

		// handle specials
		if (g_string::equals("..", name_current)) {
			if (parent->parent != 0) {
				child = parent->parent;
			}

		} else if (g_string::equals(".", name_current)) {
			// skip

		} else {
			// check if requested child exists
			child = parent->find_child(name_current);

			if (child == 0) {
				break;
			}
		}

		// TODO jump into symlinks
		/*
		 if(follow_symlinks) {
		 if (child->type == G_FILESYSTEM_NODE_SYMLINK) {
		 g_fs_virt_id mounted_node_id = child->value;

		 auto entry = nodes->get(mounted_node_id);
		 if (entry != 0) {
		 child = entry->value;
		 } else {
		 break;
		 }
		 }
		 }
		 */
	}

	*out_parent = parent;
	*out_child = child;
}

/**
 *
 */
void g_filesystem::process_closed(g_pid pid) {
	g_file_descriptor_table* table = g_file_descriptors::get_process_table(pid);

	// close each entry
	for (auto iter = table->descriptors.begin(); iter != table->descriptors.end(); ++iter) {
		g_file_descriptor_content* content = iter->value;

		auto node_entry = nodes->get(content->node_id);
		if (node_entry) {
			g_fs_close_status stat = unmap_file(pid, node_entry->value, content);

			if (stat == G_FS_CLOSE_SUCCESSFUL) {
				g_log_debug("%! successfully closed fd %i when exiting process %i", "filesystem", content->id, pid);
			} else {
				g_log_debug("%! failed to close fd %i when exiting process %i with status %i", "filesystem", content->id, pid, stat);
			}
		}
	}

	// remove all entries
	g_file_descriptors::unmap_all(pid);
}

/**
 *
 */
void g_filesystem::process_forked(g_pid source, g_pid fork) {
	g_file_descriptor_table* source_table = g_file_descriptors::get_process_table(source);

	// clone each entry
	for (auto iter = source_table->descriptors.begin(); iter != source_table->descriptors.end(); ++iter) {
		g_file_descriptor_content* content = iter->value;

		g_fs_clonefd_status stat;
		clonefd(content->id, source, content->id, fork, &stat);
		g_log_debug("%! forking cloned fd %i from process %i -> %i with status %i", "filesystem", content->id, source, fork, stat);
	}
}

/**
 *
 */
g_fd g_filesystem::map_file(g_pid pid, g_fs_node* node, int32_t open_flags, g_fd fd) {

	if (node->type == G_FS_NODE_TYPE_FILE) {
		return g_file_descriptors::map(pid, node->id, fd, open_flags);

	} else if (node->type == G_FS_NODE_TYPE_PIPE) {
		g_pipes::add_reference(node->phys_fs_id, pid);
		return g_file_descriptors::map(pid, node->id, fd, open_flags);
	}

	g_log_warn("%! tried to open a node of non-file type %i", "filesystem", node->type);
	return -1;
}

/**
 *
 */
bool g_filesystem::unmap_file(g_pid pid, g_fs_node* node, g_file_descriptor_content* fd) {

	if (node->type == G_FS_NODE_TYPE_FILE) {
		g_file_descriptors::unmap(pid, fd->id);
		return true;

	} else if (node->type == G_FS_NODE_TYPE_PIPE) {
		g_pipes::remove_reference(node->phys_fs_id, pid);
		g_file_descriptors::unmap(pid, fd->id);
		return true;
	}

	g_log_warn("%! tried to close a node of non-file type %i", "filesystem", node->type);
	return false;
}

/**
 *
 */
void g_filesystem::get_real_path_to_node(g_fs_node* node, char* out) {

	g_fs_node* current = node;

	int abs_len = 0;
	while (current != 0) {

		// on root we can stop
		if (current->type == G_FS_NODE_TYPE_ROOT) {
			break;
		}

		// check
		if (current->name == 0) {
			g_log_warn("%! problem: tried to add name of nameless node %i", "filesystem", current->id);
			break;
		}

		// get & check length + slash
		int name_len = g_string::length(current->name);
		if (abs_len + name_len + 1 > G_PATH_MAX) {
			g_log_warn("%! problem: tried to create a path thats longer than G_PATH_MAX from a node", "filesystem");
			break;
		}

		// copy so that out contains: current->name "/" out
		for (int i = abs_len; i >= 0; i--) {
			out[i + name_len + 1] = out[i];
		}
		out[name_len] = '/';
		g_memory::copy(out, current->name, name_len);

		abs_len += name_len + 1;

		current = current->parent;
	}

	// add final slash & null termination
	for (int i = abs_len; i >= 0; i--) {
		out[i + 1] = out[i];
	}
	out[0] = '/';
	out[abs_len + 1] = 0;
}

/**
 *
 */
void g_filesystem::concat_as_absolute_path(char* relative_base, char* in, char* out) {

	// check if valid input
	int len_in = g_string::length(in);
	if (len_in == 0) {
		out[0] = 0;
		return;
	}

	// if path is absolute, only copy to out
	if (in[0] == '/') {
		g_string::copy(out, in);
	} else {
		// otherwise append: cwd + "/" + in
		int len_cwd = g_string::length(relative_base);
		int len_sep = 1;
		g_memory::copy(out, relative_base, len_cwd);
		g_memory::copy(&out[len_cwd], "/", len_sep);
		g_memory::copy(&out[len_cwd + len_sep], in, len_in);
		out[len_cwd + len_in + len_sep] = 0;
	}
}

/**
 *
 */
g_fs_register_as_delegate_status g_filesystem::create_delegate(g_thread* thread, char* name, g_fs_phys_id phys_mountpoint_id, g_fs_virt_id* out_mountpoint_id,
		g_address* out_transaction_storage) {

	g_fs_node* existing = root->find_child(name);
	if (existing) {
		return G_FS_REGISTER_AS_DELEGATE_FAILED_EXISTING;
	}

	// create and prepare delegate
	g_fs_delegate_tasked* tasked_delegate = new g_fs_delegate_tasked(thread);
	if (!tasked_delegate->prepare(out_transaction_storage)) {
		delete tasked_delegate;
		g_log_info("%! failed to create delegate", "filesystem");
		return G_FS_REGISTER_AS_DELEGATE_FAILED_DELEGATE_CREATION;
	}

	// create mountpoint
	g_fs_node* mountpoint = create_node();
	mountpoint->set_delegate(tasked_delegate);
	mountpoint->name = new char[g_string::length(name) + 1];
	g_string::copy(mountpoint->name, name);
	mountpoint->type = G_FS_NODE_TYPE_MOUNTPOINT;
	mountpoint->phys_fs_id = phys_mountpoint_id;
	mount_root->add_child(mountpoint);
	G_DEBUG_INTERFACE_FILESYSTEM_UPDATE_NODE(mountpoint);

	// copy mountpoint id
	*out_mountpoint_id = mountpoint->id;

	g_log_info("%! mountpoint '%s' (node id %i) is handled by delegate task %i", "filesystem", mountpoint->name, mountpoint->id, thread->id);

	return G_FS_REGISTER_AS_DELEGATE_SUCCESSFUL;
}

/**
 *
 */
bool g_filesystem::node_for_descriptor(g_pid pid, g_fd fd, g_fs_node** out_node, g_file_descriptor_content** out_fd) {

	// find file descriptor
	g_file_descriptor_content* fd_content = g_file_descriptors::get(pid, fd);
	if (fd_content == 0) {
		return false;
	}

	// get node for id in fd
	g_fs_virt_id nodeid = fd_content->node_id;
	auto entry = nodes->get(nodeid);
	if (entry == 0) {
		return false;
	}

	*out_fd = fd_content;
	*out_node = entry->value;

	return true;
}

/**
 *
 */
g_fd g_filesystem::clonefd(g_fd source_fd, g_pid source_pid, g_fd target_fd, g_pid target_pid, g_fs_clonefd_status* out_status) {

	g_fs_node* source_node;
	g_file_descriptor_content* source_fd_content;
	if (!node_for_descriptor(source_pid, source_fd, &source_node, &source_fd_content)) {
		*out_status = G_FS_CLONEFD_INVALID_SOURCE_FD;
		return -1;
	}

	g_file_descriptor_content* target_fd_content = 0;
	g_fs_node* target_node = 0;
	if (target_fd != -1) {
		node_for_descriptor(target_pid, target_fd, &target_node, &target_fd_content);

		// close old file descriptor if available
		if (target_node) {
			unmap_file(source_pid, target_node, target_fd_content);
		}
	}

	// open new file descriptor
	g_fd created = map_file(target_pid, source_node, 0, target_fd);
	if (created != -1) {

		// clone fd contents
		g_file_descriptor_content* created_fd_content = 0;
		g_fs_node* created_node = 0;
		if (!node_for_descriptor(target_pid, created, &created_node, &created_fd_content)) {
			*out_status = G_FS_CLONEFD_ERROR;
			return -1;
		}
		source_fd_content->clone_into(created_fd_content);

		// operation fine
		*out_status = G_FS_CLONEFD_SUCCESSFUL;
		return created;
	}

	*out_status = G_FS_CLONEFD_ERROR;
	return -1;
}

g_fs_pipe_status g_filesystem::pipe(g_thread* thread, bool blocking, g_fd* out_write, g_fd* out_read) {

	g_fs_node* node = create_node();
	node->type = G_FS_NODE_TYPE_PIPE;
	node->is_blocking = blocking;
	pipe_root->add_child(node);

	node->phys_fs_id = g_pipes::create();
	*out_write = map_file(thread->process->main->id, node, 0);
	*out_read = map_file(thread->process->main->id, node, 0);
	return G_FS_PIPE_SUCCESSFUL;
}
