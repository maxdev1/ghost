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

#include <ghost.h>
#include <sstream>
#include <map>
#include <string>
#include <ghostuser/utils/logger.hpp>
#include <stdio.h>

/**
 *
 */
struct phys_node_t {
	g_fs_virt_id virt_id;
	int phys_id;
	phys_node_t* parent;
	std::string name;
};

static std::map<int, phys_node_t*> phys_nodes;

/**
 *
 */
phys_node_t* fake_discovery(phys_node_t* parent, std::string name) {
	static int fake_phys_id_counter = 1238;
	phys_node_t* node = new phys_node_t;
	node->parent = parent;
	node->phys_id = fake_phys_id_counter++;
	node->name = name;
	phys_nodes[node->phys_id] = node;
	return node;
}

/**
 *
 */
void print_phys_tree() {

	g_logger::log(" physical nodes:");
	for (auto entry : phys_nodes) {
		phys_node_t* node = entry.second;
		g_logger::log("    '" + node->name + "', v%i, p%i, parent v%i p%i",
				node->virt_id, node->phys_id,
				node->parent ? node->parent->virt_id : 0,
				node->parent ? node->parent->phys_id : 0);
	}

}

/**
 *
 */
int main(int argc, char* argv[]) {

	/**
	 * The driver must first register itself as a file system
	 * delegate by telling the kernel to create a mountpoint.
	 *
	 * The process must specify a name, and gets a mountpoint &
	 * a transaction storage back:
	 */
	const char* name = "testfs";
	g_fs_virt_id mountpoint_id;
	g_address transaction_storage_addr;

	phys_node_t* mountpoint_node = fake_discovery(0, "root");
	g_fs_register_as_delegate_status result = g_fs_register_as_delegate(name,
			mountpoint_node->phys_id, &mountpoint_id,
			&transaction_storage_addr);
	mountpoint_node->virt_id = mountpoint_id;

	/**
	 * Now the result of the registration must be checked
	 */
	if (result == G_FS_REGISTER_AS_DELEGATE_FAILED_EXISTING) {
		g_logger::log(
				"failed to register as delegate named '%s', mountpoint already exists",
				name);

	} else if (result == G_FS_REGISTER_AS_DELEGATE_FAILED_DELEGATE_CREATION) {
		g_logger::log(
				"failed to register as delegate named '%s', delegate could not be created",
				name);

	} else if (result == G_FS_REGISTER_AS_DELEGATE_SUCCESSFUL) {
		/**
		 * The transaction storage is used in various operations, like discovering nodes,
		 * to transfer data from the kernel to the process.
		 */
		uint8_t* transaction_storage = (uint8_t*) transaction_storage_addr;

		g_tid tid = g_get_tid();
		g_fs_phys_id dummy_fs_node_id_counter = 0;

		/**
		 * The driver must now make a message loop, asking for messages that are
		 * addressed to the **thread** that was registered.
		 */
		while (true) {
			g_message_empty(request);
			g_recv_msg(tid, &request);

			// Now do what is necessary to perform the requested operation
			if (request.type == G_FS_TASKED_DELEGATE_REQUEST_TYPE_DISCOVER) {
				g_fs_tasked_delegate_transaction_storage_discovery* disc =
						(g_fs_tasked_delegate_transaction_storage_discovery*) transaction_storage;

				phys_node_t* parent = 0;
				if (phys_nodes.count(disc->parent_phys_fs_id) > 0) {
					parent = phys_nodes[disc->parent_phys_fs_id];

					phys_node_t* child = fake_discovery(parent, disc->name);

					g_fs_virt_id created_vfs_node_id;
					g_fs_create_node_status stat = g_fs_create_node(
							parent->virt_id, disc->name, G_FS_NODE_TYPE_FILE,
							child->phys_id, &created_vfs_node_id);
					child->virt_id = created_vfs_node_id;
					g_logger::log(
							"discovered child of %i (phys %i) named %s, virt %i, phys %i",
							parent->virt_id, parent->phys_id, disc->name,
							child->virt_id, child->phys_id);

					disc->result_status = G_FS_DISCOVERY_SUCCESSFUL;
				} else {
					g_logger::log(
							"tried to find child of non-existing physical node: %i",
							disc->parent_phys_fs_id);
					disc->result_status = G_FS_DISCOVERY_ERROR;
				}
				g_fs_set_transaction_status(request.parameterA,
						G_FS_TRANSACTION_FINISHED);

			} else if (request.type == G_FS_TASKED_DELEGATE_REQUEST_TYPE_READ) {
				g_fs_tasked_delegate_transaction_storage_read* storage =
						(g_fs_tasked_delegate_transaction_storage_read*) transaction_storage;

				// this is a dummy method, driver has exactly 1024 random files
				int toread = 1024 - storage->offset;
				int requested = storage->length;
				toread = toread < requested ? toread : requested;
				for (int i = 0; i < toread; i++) {
					((char*) storage->mapped_buffer)[i] = (char) ('a'
							+ (i % ('z' - 'a')));
				}
				storage->result_read = toread;
				storage->result_status = G_FS_READ_SUCCESSFUL;
				g_fs_set_transaction_status(request.parameterA,
						G_FS_TRANSACTION_FINISHED);

			} else if (request.type
					== G_FS_TASKED_DELEGATE_REQUEST_TYPE_WRITE) {
				g_fs_tasked_delegate_transaction_storage_write* storage =
						(g_fs_tasked_delegate_transaction_storage_write*) transaction_storage;

				std::stringstream content;
				for (int i = 0; i < storage->length; i++) {
					content << ((char*) storage->mapped_buffer)[i];
				}
				g_logger::log(
						"wrote %i to node %i: '" + content.str() + "'",
						storage->length, storage->phys_fs_id);
				storage->result_write = storage->length;
				storage->result_status = G_FS_WRITE_SUCCESSFUL;
				g_fs_set_transaction_status(request.parameterA,
						G_FS_TRANSACTION_FINISHED);

			} else if (request.type
					== G_FS_TASKED_DELEGATE_REQUEST_TYPE_GET_LENGTH) {
				g_fs_tasked_delegate_transaction_storage_get_length* storage =
						(g_fs_tasked_delegate_transaction_storage_get_length*) transaction_storage;

				storage->result_length = 1024;
				storage->result_status = G_FS_LENGTH_SUCCESSFUL;
				g_fs_set_transaction_status(request.parameterA,
						G_FS_TRANSACTION_FINISHED);

			} else if (request.type
					== G_FS_TASKED_DELEGATE_REQUEST_TYPE_READ_DIRECTORY) {
				g_fs_tasked_delegate_transaction_storage_directory_refresh* storage =
						(g_fs_tasked_delegate_transaction_storage_directory_refresh*) transaction_storage;

				klog("got read directory request");

				for(int position = 0; position < 10; position++) {
					phys_node_t* parent = phys_nodes[storage->parent_phys_fs_id];

					// prepare name
					std::stringstream filenames;
					filenames << "file" << position;
					std::string filename = filenames.str();

					// find existing node at position
					phys_node_t* child = 0;

					int index = 0;
					for (auto entry : phys_nodes) {
						if (entry.second->parent->phys_id == parent->phys_id) {
							if (index == position) {
								child = entry.second;
								break;
							}
							++index;
						}
					}

					// otherwise create node
					if (child == 0) {
						child = fake_discovery(parent, filename.c_str());
						klog("- creating fake child: %s", filename.c_str());
					}

					klog("- fake child: %s", filename.c_str());

					// create/update node
					g_fs_virt_id vfs_node_id;
					g_fs_create_node_status stat = g_fs_create_node(
							parent->virt_id, (char*) filename.c_str(),
							G_FS_NODE_TYPE_FILE, child->phys_id, &vfs_node_id);
					child->virt_id = vfs_node_id;

					if (stat == G_FS_CREATE_NODE_STATUS_CREATED) {
						g_logger::log("created node when reading dir");
					} else if (stat == G_FS_CREATE_NODE_STATUS_UPDATED) {
						g_logger::log("updated node when reading dir");
					} else {
						g_logger::log("failed on node when reading dir");
					}

					storage->result_status = G_FS_READ_DIRECTORY_SUCCESSFUL;
				}

				g_fs_set_transaction_status(request.parameterA,
						G_FS_TRANSACTION_FINISHED);
			}

		}

	} else {
		g_logger::log(
				"failed to register as delegate named '%s', unknown error occured",
				name);
	}

	return 0;
}
