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

#include <memory/collections/address_stack.hpp>
#include <memory/kernel_heap.hpp>
#include <logger/logger.hpp>
#include <kernel.hpp>

/**
 *
 */
g_address_stack::g_address_stack() {
	current = 0;
	position = 0;
}

/**
 *
 */
void g_address_stack::push(g_address address) {
	if (current == 0) {
		current = new g_address_stack_chunk;
		current->previous = 0;
		current->next = 0;
	}

	current->entries[position] = address;
	++position;

	if (position == ADDRESS_STACK_CHUNK_ENTRIES) {
		if (current->next == 0) {
			// Allocate new frame
			current->next = new g_address_stack_chunk;
			current->next->previous = current;
		}
		current = current->next;

		position = 0;
	}
}

/**
 *
 */
g_address g_address_stack::pop() {

	// This happens if the last push exceeded a frame
	if (position == 0) {
		position = ADDRESS_STACK_CHUNK_ENTRIES;
		current = current->previous;
	}

	if (current == 0) {
		g_log_warn("%! critical: no pages left in address stack", "addrstack");
		return 0;
	}

	--position;
	uint32_t address = current->entries[position];

	if (position == 0) {
		current = current->previous;
		position = ADDRESS_STACK_CHUNK_ENTRIES - 1;

		if (current != 0) {
			// Delete unused frame
			delete current->next;
			current->next = 0;
		}
	}

	return address;
}
