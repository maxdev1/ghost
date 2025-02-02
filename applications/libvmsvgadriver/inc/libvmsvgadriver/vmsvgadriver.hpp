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

#ifndef __LIBVMSVGADRIVER_VMSVGADRIVER__
#define __LIBVMSVGADRIVER_VMSVGADRIVER__

#include <stdint.h>

#define G_VMSVGA_DRIVER_IDENTIFIER			"vmsvgadriver"

struct g_vmsvga_mode_info
{
    uint16_t resX;
    uint16_t resY;
    uint16_t bpp;
    uint16_t bpsl;
    uint32_t lfb;
}__attribute__((packed));

typedef int g_vmsvga_command;
#define G_VMSVGA_COMMAND_SET_MODE	((g_vmsvga_command) 0)
#define G_VMSVGA_COMMAND_UPDATE 	((g_vmsvga_command) 1)

/**
 *
 */
struct g_vmsvga_request_header
{
    g_vmsvga_command command;
}__attribute__((packed));

typedef int g_vmsvga_set_mode_status;
#define G_VMSVGA_SET_MODE_STATUS_SUCCESS		((g_vmsvga_set_mode_status) 0)
#define G_VMSVGA_SET_MODE_STATUS_FAILED		    ((g_vmsvga_set_mode_status) 1)

/**
 *
 */
struct g_vmsvga_set_mode_request
{
    g_vmsvga_request_header header;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
}__attribute__((packed));

struct g_vmsvga_set_mode_response
{
    g_vmsvga_set_mode_status status;
    g_vmsvga_mode_info mode_info;
}__attribute__((packed));

/**
 * Sends a request to the VBE driver and attempts to set the best matching available
 * video mode using the VESA graphics interface.
 */
bool vmsvgaDriverSetMode(uint16_t width, uint16_t height, uint8_t bpp, g_vmsvga_mode_info& out);

/**
 *
 */
struct g_vmsvga_update_request
{
    g_vmsvga_request_header header;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
}__attribute__((packed));

/**
 * Sends the instruction to update a screen region to the driver.
 */
void vmsvgaDriverUpdate(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

#endif
