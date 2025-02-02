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
#include <libvmsvgadriver/vmsvgadriver.hpp>
#include <cstdio>
g_tid driverTid = -1;

bool vmsvgaDriverSetMode(uint16_t width, uint16_t height, uint8_t bpp, g_vmsvga_mode_info& out)
{
	if(driverTid == G_TID_NONE)
	{
		driverTid = g_task_get_id(G_VMSVGA_DRIVER_IDENTIFIER);

		if(driverTid == G_TID_NONE)
		{
			return false;
		}
	}

	g_message_transaction transaction = g_get_message_tx_id();

	g_vmsvga_set_mode_request request;
	request.header.command = G_VMSVGA_COMMAND_SET_MODE;
	request.width = width;
	request.height = height;
	request.bpp = bpp;
	g_send_message_t(driverTid, &request, sizeof(g_vmsvga_set_mode_request), transaction);

	size_t buflen = sizeof(g_message_header) + sizeof(g_vmsvga_set_mode_response);
	uint8_t buf[buflen];
	auto status = g_receive_message_t(buf, buflen, transaction);
	g_vmsvga_set_mode_response* response = (g_vmsvga_set_mode_response*) G_MESSAGE_CONTENT(buf);

	if(status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		if(response->status == G_VMSVGA_SET_MODE_STATUS_SUCCESS)
		{
			out = response->mode_info;
			return true;
		}
	}
	return false;
}

void vmsvgaDriverUpdate(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
	g_message_transaction transaction = g_get_message_tx_id();

	g_vmsvga_update_request request;
	request.header.command = G_VMSVGA_COMMAND_UPDATE;
	request.x = x;
	request.y = y;
	request.width = width;
	request.height = height;
	g_send_message_t(driverTid, &request, sizeof(g_vmsvga_update_request), transaction);
}
