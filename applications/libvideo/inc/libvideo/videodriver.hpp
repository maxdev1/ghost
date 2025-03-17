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

#ifndef __LIBVIDEO_VIDEODRIVER__
#define __LIBVIDEO_VIDEODRIVER__

#include <stdint.h>
#include <ghost/tasks/types.h>
#include <libdevice/interface.hpp>

struct g_video_mode_info
{
    uint16_t resX;
    uint16_t resY;
    uint16_t bpp;
    uint16_t bpsl;
    uint32_t lfb;
    bool explicit_update;
}__attribute__((packed));

typedef int g_video_command;
#define G_VIDEO_COMMAND_SET_MODE	((g_video_command) 0)
#define G_VIDEO_COMMAND_UPDATE 	    ((g_video_command) 1)

/**
 *
 */
struct g_video_request_header
{
    g_video_command command;
    g_device_id device;
}__attribute__((packed));

typedef int g_video_set_mode_status;
#define G_VIDEO_SET_MODE_STATUS_SUCCESS		    ((g_video_set_mode_status) 0)
#define G_VIDEO_SET_MODE_STATUS_FAILED		    ((g_video_set_mode_status) 1)

/**
 *
 */
struct g_video_set_mode_request
{
    g_video_request_header header;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
}__attribute__((packed));

struct g_video_set_mode_response
{
    g_video_set_mode_status status;
    g_video_mode_info mode_info;
}__attribute__((packed));

bool videoDriverSetMode(g_tid driverTid, g_device_id device, uint16_t width, uint16_t height, uint8_t bpp,
                        g_video_mode_info& out);

/**
 *
 */
struct g_video_update_request
{
    g_video_request_header header;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
}__attribute__((packed));

/**
 * If the video driver requires explicit updates, calls the driver to perform an update of the given area.
 */
void videoDriverUpdate(g_tid driverTid, g_device_id device, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

#endif
