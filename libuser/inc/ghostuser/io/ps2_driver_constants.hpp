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

#include <ghost.h>
#include <ghost/types.h>

#define G_PS2_DRIVER_IDENTIFIER							"ps2driver"

/**
 * Shared memory area that is used for transferring PS/2 input data
 * to a process that handles this data.
 */
typedef struct {
	struct {
		/**
		 * Lock to use when accessing any of the variables in the structure.
		 */
		g_atom lock;

		/**
		 * This atom is set to true when the PS2 driver is initialized. Once
		 * the driver writes bytes to the buffer, it is set to false.
		 *
		 * A client shall block on this atom when reading data. Once the atom is
		 * false, the client shall take the information in this struct. The
		 * client then shall set this atom to true to lock himself.
		 */
		g_atom buffer_empty_lock;

		int16_t move_x;
		int16_t move_y;
		uint16_t flags;
	} mouse;

	struct {
		/**
		 * This atom is set to true when the PS2 driver is initialized. Once
		 * the driver writes bytes to the buffer, it is set to false.
		 *
		 * A client shall block on this atom when reading data. Once the atom is
		 * false, the client shall read bytes from the buffer, check if there
		 * are still bytes in the buffer and set the remaining variables
		 * accordingly.
		 */
		g_atom buffer_empty_lock;

		/**
		 * Counter for the number of bytes in the key buffer. See the lock
		 * above for a detailed description. The "buffer_amount_lock" is used
		 * for atomic access to the buffer amount counter.
		 */
		uint16_t buffer_amount;
		g_atom buffer_amount_lock;
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
	g_fd keyboard_pipe;
}__attribute__((packed)) g_ps2_register_response;

#endif
