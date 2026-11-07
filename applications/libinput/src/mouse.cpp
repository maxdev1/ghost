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

#include <ghost.h>
#include <libps2driver/ps2driver.hpp>

#include "libinput/mouse/mouse.hpp"

bool g_mouse::readMouse(g_fd in, g_mouse_info& info)
{
	g_ps2_mouse_packet packet;
	size_t offset = 0;
	while(offset < sizeof(packet))
	{
		g_fs_read_status status;
		const int32_t count = g_read_s(in, (uint8_t*) &packet + offset, sizeof(packet) - offset, &status);
		if(status == G_FS_READ_BUSY)
		{
			g_sleep(1);
			continue;
		}
		if(count <= 0 || status != G_FS_READ_SUCCESSFUL)
			return false;
		offset += count;
	}
	info.x = packet.x;
	info.y = packet.y;
	info.button1 = (packet.flags & (1 << 0));
	info.button2 = (packet.flags & (1 << 1));
	info.button3 = (packet.flags & (1 << 2));
	info.scroll = packet.scroll;
	info.millis = packet.millis;
	return true;
}
