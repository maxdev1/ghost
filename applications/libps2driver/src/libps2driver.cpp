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

#include "libps2driver/ps2driver.hpp"
#include <ghost.h>

bool ps2DriverInitialize(g_fd* keyboardReadOut, g_fd* mouseReadOut, g_tid keyboardPartnerTask, g_tid mousePartnerTask)
{

	g_tid driver_tid = g_task_get_id(G_PS2_DRIVER_IDENTIFIER);
	if(driver_tid == -1)
	{
		return false;
	}
	g_message_transaction transaction = g_get_message_tx_id();

	g_ps2_initialize_request request;
	request.header.command = G_PS2_COMMAND_INITIALIZE;
	request.keyboardPartnerTask = keyboardPartnerTask;
	request.mousePartnerTask = mousePartnerTask;
	g_send_message_t(driver_tid, &request, sizeof(g_ps2_initialize_request), transaction);

	size_t buflen = sizeof(g_message_header) + sizeof(g_ps2_initialize_response);
	uint8_t buf[buflen];
	auto status = g_receive_message_t(buf, buflen, transaction);
	g_ps2_initialize_response* response = (g_ps2_initialize_response*) G_MESSAGE_CONTENT(buf);

	if(status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		if(response->status == G_PS2_INITIALIZE_SUCCESS)
		{
			*keyboardReadOut = response->keyboardRead;
			*mouseReadOut = response->mouseRead;
			return true;
		}
	}

	return false;
}
