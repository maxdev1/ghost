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

#ifndef __LIBPS2DRIVER_PS2DRIVER__
#define __LIBPS2DRIVER_PS2DRIVER__

#include <stdint.h>
#include <ghost.h>

#define G_PS2_DRIVER_IDENTIFIER		"ps2driver"

/**
 *
 */
typedef int g_ps2_command;
#define G_PS2_COMMAND_INITIALIZE	((g_ps2_command) 0)

/**
 *
 */
struct g_ps2_request_header {
	g_ps2_command command;
}__attribute__((packed));

/**
 *
 */
typedef int g_ps2_initialize_status;
#define G_PS2_INITIALIZE_SUCCESS		((g_ps2_initialize_status) 0)
#define G_PS2_INITIALIZE_FAILED 		((g_ps2_initialize_status) 1)

/**
 *
 */
struct g_ps2_initialize_request {
	g_ps2_request_header header;
}__attribute__((packed));

/**
 *
 */
struct g_ps2_initialize_response {
    g_ps2_initialize_status status;
	g_fd keyboardRead;
	g_fd mouseRead;
}__attribute__((packed));

/**
 *
 */
struct g_ps2_mouse_packet {
	int16_t x;
    int16_t y;
    uint8_t flags;
}__attribute__((packed));

/**
 * Sends a request to the PS2 driver to allow this process to read data.
 */
bool ps2DriverInitialize(g_fd* keyboardReadOut, g_fd* mouseReadOut);

#endif
