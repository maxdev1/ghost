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

#ifndef GHOSTLIBRARY_IO_PS2DRIVERCONSTANTS
#define GHOSTLIBRARY_IO_PS2DRIVERCONSTANTS

#include <stdint.h>

#define G_PS2_DRIVER_IDENTIFIER							"ps2driver"

/**
 * Shared memory area that is used for transferring PS/2 input data
 * to a process that handles this data.
 */
typedef struct {
	struct {
		/**
		 * This atom remains set, until the driver has data to transfer.
		 * The driver delegate must wait until the atom is unset, and then
		 * set it again.
		 */
		uint8_t atom_nothing_queued;

		/**
		 * This atom is set by the driver, once it has written data that
		 * the delegate is yet to read. Once the data is read, the delegate
		 * must unset the atom.
		 */
		uint8_t atom_unhandled;

		int16_t move_x;
		int16_t move_y;
		uint16_t flags;
	} mouse;

	struct {
		uint8_t atom_nothing_queued;
		uint8_t atom_unhandled;

		uint8_t scancode;
	} keyboard;
}__attribute__((packed)) g_ps2_shared_area;

/**
 * Request sent to register the sender thread as the
 * PS/2 data handler.
 */
typedef struct {
	// empty request
}__attribute__((packed)) g_ps2_register_request;

/**
 * Response sent to a registering thread, containing the
 * shared memory area to use.
 */
typedef struct {
	g_ps2_shared_area* area;
}__attribute__((packed)) g_ps2_register_response;

#endif
