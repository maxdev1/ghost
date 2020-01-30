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

#include <ghostuser/io/ps2.hpp>
#include <ghost.h>
#include <stdio.h>
#include <ghostuser/utils/local.hpp>

g_ps2_shared_area* g_ps2_area = 0;
bool g_ps2_is_registered = false;
g_fd g_ps2_keyboard_pipe = -1;

static uint8_t g_ps2_registration_lock = false;

/**
 *
 */
bool g_ps2::registerSelf() {

	g_atomic_lock(&g_ps2_registration_lock);

	if (g_ps2_is_registered) {
		return true;
	}

	g_tid driver_tid = g_task_get_id(G_PS2_DRIVER_IDENTIFIER);
	if (driver_tid == -1) {
		klog("PS/2 driver registration failed: failed to identify PS/2 driver instance");
		g_ps2_registration_lock = false;
		return false;
	}

	g_message_transaction transaction = g_get_message_tx_id();

	// send request
	g_ps2_register_request request;
	if (g_send_message_t(driver_tid, &request, sizeof(g_ps2_register_request), transaction) != G_MESSAGE_SEND_STATUS_SUCCESSFUL) {
		klog("PS/2 driver registration error: failed to send registration request message");
		g_ps2_registration_lock = false;
		return false;
	}

	// read response
	uint32_t response_buf_len = sizeof(g_message_header) + sizeof(g_ps2_register_response);
	g_local<uint8_t> response_buf(new uint8_t[response_buf_len]);
	if (g_receive_message_t(response_buf(), response_buf_len, transaction) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		klog("PS/2 driver registration error: failed to receive registration response message");
		g_ps2_registration_lock = false;
		return false;
	}

	// receive content
	g_ps2_register_response* response = (g_ps2_register_response*) G_MESSAGE_CONTENT(response_buf());
	if (response->area == 0) {
		klog("PS/2 driver registration error: shared memory was NULL");
		g_ps2_registration_lock = false;
		return false;
	}

	// all fine
	g_ps2_registration_lock = false;
	g_ps2_is_registered = true;
	g_ps2_area = response->area;
	g_ps2_keyboard_pipe = response->keyboard_pipe;
	return true;
}
