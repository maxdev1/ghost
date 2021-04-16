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

#ifndef GHOSTLIBRARY_GRAPHICS_VBE
#define GHOSTLIBRARY_GRAPHICS_VBE

#include <stdint.h>

#define G_VBE_DRIVER_IDENTIFIER			"vbedriver"

/**
 *
 */
struct g_vbe_mode_info {
	uint16_t resX;
	uint16_t resY;
	uint16_t bpp;
	uint16_t bpsl;
	uint32_t lfb;
}__attribute__((packed));

/**
 *
 */
typedef int g_vbe_command;
#define G_VBE_COMMAND_SET_MODE	((g_vbe_command) 0)

/**
 *
 */
struct g_vbe_request_header {
	g_vbe_command command;
}__attribute__((packed));

/**
 *
 */
typedef int g_vbe_set_mode_status;
#define G_VBE_SET_MODE_STATUS_SUCCESS		((g_vbe_set_mode_status) 0)
#define G_VBE_SET_MODE_STATUS_FAILED		((g_vbe_set_mode_status) 1)

/**
 *
 */
struct g_vbe_set_mode_request {
	g_vbe_request_header header;
	uint16_t width;
	uint16_t height;
	uint8_t bpp;
}__attribute__((packed));

/**
 *
 */
struct g_vbe_set_mode_response {
	g_vbe_set_mode_status status;
	g_vbe_mode_info mode_info;
}__attribute__((packed));

/**
 * Sends a request to the VBE driver and attempts to set the best matching available
 * video mode using the VESA graphics interface.
 */
bool vbeSetMode(uint16_t width, uint16_t height, uint8_t bpp, g_vbe_mode_info& out);

#endif
