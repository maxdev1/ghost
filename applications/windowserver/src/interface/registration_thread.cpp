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

#include "registration_thread.hpp"
#include <ghostuser/ui/interface_specification.hpp>
#include <interface/command_message_receiver_thread.hpp>
#include <stdio.h>

/**
 *
 */
void registration_thread_t::run() {

	g_tid tid = g_get_tid();

	// register this thread
	if (!g_task_register_id(G_UI_REGISTRATION_THREAD_IDENTIFIER)) {
		klog("failed to register task identifier for registration thread");
		return;
	}

	// wait for initialization requests
	size_t bufferLength = sizeof(g_message_header) + sizeof(g_ui_initialize_request);
	uint8_t* buffer = new uint8_t[bufferLength];

	klog("waiting for registration requests");
	while (true) {
		g_message_receive_status stat = g_receive_message(buffer, bufferLength);

		if (stat == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			g_message_header* request_message = (g_message_header*) buffer;
			g_ui_initialize_request* request = (g_ui_initialize_request*) G_MESSAGE_CONTENT(buffer);

			// create handler thread
			command_message_receiver_thread_t* communicator = new command_message_receiver_thread_t();
			g_tid communicator_tid = communicator->start();

			// send response
			g_ui_initialize_response response;
			response.header.id = G_UI_PROTOCOL_INITIALIZATION;
			response.status = G_UI_PROTOCOL_SUCCESS;
			response.window_server_delegate_thread = communicator_tid;
			g_send_message_t(request_message->sender, &response, sizeof(g_ui_initialize_response), request_message->transaction);
		}
	}
}
