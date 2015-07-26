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
#include <ghostuser/graphics/vbe.hpp>
#include <ghostuser/tasking/ipc.hpp>
#include <ghostuser/utils/logger.hpp>

/**
 *
 */
bool g_vbe::setMode(uint16_t width, uint16_t height, uint8_t bpp, g_vbe_mode_info& out) {

	// identify vbe driver
	g_tid driver_tid = g_task_get_id(G_VBE_DRIVER_IDENTIFIER);
	if (driver_tid == -1) {
		return false;
	}
	uint32_t transaction = g_ipc_next_topic();

	// create mode-setting request
	g_message_empty(request);
	request.topic = transaction;
	request.type = G_VBE_DRIVER_COMMAND_SET_MODE;
	request.parameterA = (((uint16_t) width) << 16) | (((uint16_t) height) & 0xFFFF);
	request.parameterB = bpp;
	g_send_msg(driver_tid, &request);

	// receive response
	g_message_empty(response);
	g_recv_topic_msg(g_get_tid(), transaction, &response);

	if (response.type == G_VBE_DRIVER_COMMAND_SET_MODE) {
		out.lfb = response.parameterA;
		out.resX = response.parameterB >> 16;
		out.resY = response.parameterB & 0xFFFF;
		out.bpp = response.parameterC;
		out.bpsl = response.parameterD;
		return true;
	}
	return false;
}

