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

#include <calls/syscall_handler.hpp>

#include <kernel.hpp>
#include <ramdisk/ramdisk.hpp>
#include <logger/logger.hpp>

/**
 * Searches for a node on the ramdisk, using the absolute path of the node.
 */
G_SYSCALL_HANDLER(ramdisk_find) {
	g_syscall_ramdisk_find* data = (g_syscall_ramdisk_find*) G_SYSCALL_DATA(current_thread->cpuState);

	g_ramdisk_entry* entry = g_kernel::ramdisk->findAbsolute(data->path);
	if (entry) {
		data->nodeId = entry->id;
	} else {
		data->nodeId = -1;
	}

	return current_thread;
}

/**
 * Searches for a child with a specific name in the parent node with the given id.
 */
G_SYSCALL_HANDLER(ramdisk_find_child) {
	g_syscall_ramdisk_find_child* data = (g_syscall_ramdisk_find_child*) G_SYSCALL_DATA(current_thread->cpuState);
	data->nodeId = -1;

	g_ramdisk_entry* parent = g_kernel::ramdisk->findById(data->parentId);
	if (parent) {

		g_ramdisk_entry* entry = g_kernel::ramdisk->findRelative(parent, data->childName);
		if (entry) {
			data->nodeId = entry->id;
		}
	}

	return current_thread;
}

/**
 * Returns information to a specific node on the ramdisk file system.
 */
G_SYSCALL_HANDLER(ramdisk_info) {
	g_syscall_ramdisk_info* data = (g_syscall_ramdisk_info*) G_SYSCALL_DATA(current_thread->cpuState);

	g_ramdisk_entry* entry = g_kernel::ramdisk->findById(data->nodeId);
	if (entry) {
		data->length = entry->datalength;

		uint32_t nameLength = g_string::length(entry->name);
		g_memory::copy(data->name, entry->name, nameLength);
		data->name[nameLength] = 0;

		data->type = entry->type;
	} else {
		data->type = G_RAMDISK_ENTRY_TYPE_UNKNOWN;
	}

	return current_thread;
}

/**
 * Reads from a ramdisk node.
 */
G_SYSCALL_HANDLER(ramdisk_read) {
	g_syscall_ramdisk_read* data = (g_syscall_ramdisk_read*) G_SYSCALL_DATA(current_thread->cpuState);

	g_ramdisk_entry* entry = g_kernel::ramdisk->findById(data->nodeId);
	if (entry && entry->type == G_RAMDISK_ENTRY_TYPE_FILE) {
		int bytesFromOffset = entry->datalength - ((int32_t) data->offset);
		if (bytesFromOffset < 0) {
			bytesFromOffset = 0;
		}

		int byteCount = (((long) bytesFromOffset) > (long) data->length) ? data->length : bytesFromOffset;
		if (byteCount > 0) {
			for (uint32_t i = 0; i < data->length; i++) {
				data->buffer[i] = 0;
			}

			g_memory::copy(data->buffer, &entry->data[data->offset], byteCount);
		}

		data->readBytes = byteCount;
	} else {
		data->readBytes = -1;
	}

	return current_thread;
}

/**
 * Returns the number of children a ramdisk node has.
 */
G_SYSCALL_HANDLER(ramdisk_child_count) {
	g_syscall_ramdisk_child_count* data = (g_syscall_ramdisk_child_count*) G_SYSCALL_DATA(current_thread->cpuState);

	data->count = g_kernel::ramdisk->getChildCount(data->nodeId);

	return current_thread;
}

/**
 * Returns the id of the child of the parent node (with the given id) at the given index on the ramdisk.
 */
G_SYSCALL_HANDLER(ramdisk_child_at) {
	g_syscall_ramdisk_child_at* data = (g_syscall_ramdisk_child_at*) G_SYSCALL_DATA(current_thread->cpuState);

	g_ramdisk_entry* entry = g_kernel::ramdisk->getChildAt(data->nodeId, data->index);

	if (entry) {
		data->childId = entry->id;
	} else {
		data->childId = -1;
	}

	return current_thread;
}
