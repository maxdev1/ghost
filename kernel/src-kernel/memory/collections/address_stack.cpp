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

	// first ever? create current
	if (current == 0) {
		current = new g_address_stack_frame;
	}

	// put address on frame
	current->entries[position++] = address;

	// create new frame when at the end
	if (position == G_ADDRESS_STACK_FRAME_ENTRIES) {
		current->next = new g_address_stack_frame;
		current->next->previous = current;
		current = current->next;
		position = 0;
	}
}

/**
 *
 */
g_address g_address_stack::pop() {

	// if no frames, return nothing
	if (current == 0) {
		return 0;
	}

	// if at foot of frame, try to jump back
	if(position == 0) {

		// return nothing if no previous frame
		if(current->previous == 0) {
			return 0;
		}

		// remove frame
		current = current->previous;
		delete current->next;
		current->next = 0;

		position = G_ADDRESS_STACK_FRAME_ENTRIES;
	}

	return current->entries[--position];
}
