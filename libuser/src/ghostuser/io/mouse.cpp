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
#include <ghostuser/io/mouse.hpp>
#include <ghostuser/io/ps2_driver_constants.hpp>
#include <ghostuser/tasking/ipc.hpp>
#include <ghostuser/utils/logger.hpp>

static uint32_t mouseTopic = -1;

/**
 *
 */
void g_mouse::registerMouse() {

	uint32_t ps2driverid = g_task_get_id(G_PS2_DRIVER_IDENTIFIER);
	if (ps2driverid == -1) {
		return;
	}

	mouseTopic = g_ipc_next_topic();

	g_ps2_register_request request;
	request.command = G_PS2_COMMAND_REGISTER_MOUSE;
	g_send_message_t(ps2driverid, &request, sizeof(g_ps2_register_request), mouseTopic);
}

/**
 *
 */
g_mouse_info g_mouse::readMouse() {

	if (mouseTopic == -1) {
		registerMouse();
	}

	size_t buflen = sizeof(g_message_header) + sizeof(g_ps2_mouse_packet);
	uint8_t buf[buflen];

	g_mouse_info e;
	if (g_receive_message_t(buf, buflen, mouseTopic) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ps2_mouse_packet* packet = (g_ps2_mouse_packet*) G_MESSAGE_CONTENT(buf);

		e.x = packet->x;
		e.y = packet->y;
		e.button1 = (packet->flags & (1 << 0));
		e.button2 = (packet->flags & (1 << 1));
		e.button3 = (packet->flags & (1 << 2));
	}
	return e;
}
