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

#include "ps2driver.hpp"

#include <libps2/ps2.hpp>

#include <ghost.h>
#include <stdint.h>
#include <stdio.h>

g_fd keyboardRead;
g_fd keyboardWrite;
g_fd mouseRead;
g_fd mouseWrite;

g_tid keyboardPartnerTask = G_TID_NONE;
g_tid mousePartnerTask = G_TID_NONE;

int main()
{
	if(!g_task_register_id(G_PS2_DRIVER_IDENTIFIER))
	{
		klog("ps2driver: could not register with task identifier '%s'", (char*) G_PS2_DRIVER_IDENTIFIER);
		return -1;
	}

	ps2DriverInitialize();
	ps2DriverReceiveMessages();
	return 0;
}

void ps2DriverInitialize()
{
	if(g_pipe_b(&keyboardWrite, &keyboardRead, false) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("ps2driver: failed to open pipe for keyboard data");
		return;
	}

	if(g_pipe_b(&mouseWrite, &mouseRead, false) != G_FS_PIPE_SUCCESSFUL)
	{
		klog("ps2driver: failed to open pipe for mouse data");
		return;
	}

	ps2Initialize(ps2MouseCallback, ps2KeyboardCallback);
}

void ps2MouseCallback(int16_t x, int16_t y, uint8_t flags)
{
	g_ps2_mouse_packet packet;
	packet.x = x;
	packet.y = y;
	packet.flags = flags;
	g_write(mouseWrite, &packet, sizeof(g_ps2_mouse_packet));

	if(mousePartnerTask != G_TID_NONE)
		g_yield_t(mousePartnerTask);
}

void ps2KeyboardCallback(uint8_t c)
{
	if(c == 0x3f) // F5
	{
		g_dump();
	}

	g_write(keyboardWrite, &c, 1);

	if(keyboardPartnerTask != G_TID_NONE)
		g_yield_t(keyboardPartnerTask);
}

void ps2DriverReceiveMessages()
{
	size_t buflen = sizeof(g_message_header) + sizeof(g_ps2_initialize_request) /*TODO*/;
	uint8_t buf[buflen];

	for(;;)
	{
		auto status = g_receive_message(buf, buflen);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			klog("error receiving message, retrying");
			continue;
		}

		g_message_header* header = (g_message_header*) buf;
		g_ps2_request_header* request = (g_ps2_request_header*) G_MESSAGE_CONTENT(buf);

		if(request->command == G_PS2_COMMAND_INITIALIZE)
		{
			ps2HandleCommandInitialize((g_ps2_initialize_request*) request, header->sender, header->transaction);
		}
		else
		{
			klog("vbedriver: received unknown command %i from task %i", request->command, header->sender);
		}
	}
}

void ps2HandleCommandInitialize(g_ps2_initialize_request* request, g_tid requestingTaskId,
                                g_message_transaction requestTransaction)
{
	g_pid targetPid = g_get_pid_for_tid(requestingTaskId);
	g_pid sourcePid = g_get_pid();

	keyboardPartnerTask = request->keyboardPartnerTask;
	mousePartnerTask = request->mousePartnerTask;

	g_ps2_initialize_response response;
	response.status = G_PS2_STATUS_SUCCESS;
	response.keyboardRead = g_clone_fd(keyboardRead, sourcePid, targetPid);
	response.mouseRead = g_clone_fd(mouseRead, sourcePid, targetPid);

	g_send_message_t(requestingTaskId, &response, sizeof(g_ps2_initialize_response), requestTransaction);
}
