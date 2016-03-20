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

#include <filesystem/fs_descriptors.hpp>
#include <logger/logger.hpp>

static g_hash_map<g_pid, g_file_descriptor_table*>* process_descriptor_table_map;

/**
 *
 */
void g_file_descriptors::initialize() {
	process_descriptor_table_map = new g_hash_map<g_pid, g_file_descriptor_table*>();
}

/**
 *
 */
g_file_descriptor_content* g_file_descriptors::create_descriptor(g_file_descriptor_table* table, g_fd override_fd) {
	g_fd descriptor;
	if (override_fd == -1) {
		descriptor = table->nextFileDescriptor++;
	} else {
		descriptor = override_fd;
	}

	g_file_descriptor_content* desc = new g_file_descriptor_content;
	desc->id = descriptor;
	desc->offset = 0;
	table->descriptors.put(descriptor, desc);
	return desc;
}

/**
 *
 */
g_fd g_file_descriptors::map(g_pid pid, g_fs_virt_id node_id, g_fd fd, int32_t open_flags) {

	g_file_descriptor_table* process_table = get_process_table(pid);

	g_file_descriptor_content* desc = create_descriptor(process_table, fd);
	desc->node_id = node_id;
	desc->open_flags = open_flags;

	return desc->id;
}

/**
 *
 */
void g_file_descriptors::unmap(g_pid pid, g_fd fd) {

	g_file_descriptor_table* process_table = get_process_table(pid);
	auto entry = process_table->descriptors.get(fd);
	if (entry) {
		auto content = entry->value;
		process_table->descriptors.remove(content->id);
		delete content;
	}
}

/**
 *
 */
void g_file_descriptors::unmap_all(g_pid pid) {

	g_file_descriptor_table* process_table = get_process_table(pid);

	for (auto iter = process_table->descriptors.begin(); iter != process_table->descriptors.end(); ++iter) {
		g_file_descriptor_content* content = iter->value;
		delete content;
	}

	process_descriptor_table_map->remove(pid);
	delete process_table;
}

/**
 *
 */
g_file_descriptor_table* g_file_descriptors::get_process_table(g_pid pid) {

	auto entry = process_descriptor_table_map->get(pid);

	g_file_descriptor_table* table = 0;
	if (entry == 0) {
		table = new g_file_descriptor_table;
		process_descriptor_table_map->put(pid, table);
	} else {
		table = entry->value;
	}

	return table;
}

/**
 *
 */
g_file_descriptor_content* g_file_descriptors::get(g_pid pid, g_fd fd) {

	g_file_descriptor_table* process_table = get_process_table(pid);
	auto entry = process_table->descriptors.get(fd);
	if (entry) {
		return entry->value;
	}
	return 0;
}
