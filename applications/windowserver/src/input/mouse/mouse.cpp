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

#include "input/mouse/mouse.hpp"
#include <ghost.h>
#include <libps2driver/ps2driver.hpp>

g_mouse_info g_mouse::readMouse(g_fd in)
{
    g_ps2_mouse_packet packet;

    g_mouse_info e;
    if(g_read(in, &packet, sizeof(g_ps2_mouse_packet)) == sizeof(g_ps2_mouse_packet))
    {
        e.x = packet.x;
        e.y = packet.y;
        e.button1 = (packet.flags & (1 << 0));
        e.button2 = (packet.flags & (1 << 1));
        e.button3 = (packet.flags & (1 << 2));
    }
    return e;
}
