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
#include "system/serial/serial_port.hpp"

// only required in full interface mode
#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL

/**
 *
 */
void g_debug_interface_kernel::publishTaskStatus(int tid, const char* status) {

	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED;

	g_debug_interface::writeShort(G_DEBUG_MESSAGE_TASK_SET_STATUS);
	g_debug_interface::writeInt(tid);
	g_debug_interface::writeString(status);
}

/**
 *
 */
void g_debug_interface_kernel::publishTaskRounds(int tid, long rounds) {

	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED;

	g_debug_interface::writeShort(G_DEBUG_MESSAGE_TASK_SET_ROUNDS);
	g_debug_interface::writeInt(tid);
	g_debug_interface::writeLong(rounds);
}

/**
 *
 */
void g_debug_interface_kernel::taskSetIdentifier(int tid, const char* identifier) {

	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	g_debug_interface::writeShort(G_DEBUG_MESSAGE_TASK_SET_IDENTIFIER);
	g_debug_interface::writeInt(tid);
	g_debug_interface::writeString(identifier);
}

/**
 *
 */
void g_debug_interface_kernel::taskSetSourcePath(int tid, const char* source_path) {

	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	g_debug_interface::writeShort(G_DEBUG_MESSAGE_TASK_SET_SOURCE_PATH);
	g_debug_interface::writeInt(tid);
	g_debug_interface::writeString(source_path);
}

/**
 *
 */
void g_debug_interface_kernel::filesystemUpdateNode(g_fs_node* node) {

	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	g_debug_interface::writeShort(G_DEBUG_MESSAGE_FILESYSTEM_UPDATE_NODE);
	g_debug_interface::writeInt(node->id);
	g_debug_interface::writeInt(node->phys_fs_id);
	g_debug_interface::writeInt(node->parent != 0 ? node->parent->id : -1);
	g_debug_interface::writeString((const char*) node->name);
}

/**
 *
 */
void g_debug_interface_kernel::memorySetPageUsage(uint32_t pageaddr, int usage) {

	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	g_debug_interface::writeShort(G_DEBUG_MESSAGE_MEMORY_SET_PAGE_USAGE);
	g_debug_interface::writeInt(pageaddr);
	g_debug_interface::writeByte(usage);
}

/**
 *
 */
void g_debug_interface_kernel::systemInformation(const char* key, uint64_t value) {

	G_DEBUG_INTERFACE_EXIT_IF_UNINITIALIZED

	g_debug_interface::writeShort(G_DEBUG_MESSAGE_SYSTEM_INFORMATION);
	g_debug_interface::writeString(key);
	g_debug_interface::writeLong(value);
}

#endif
