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

#include <libwindow/interface.hpp>
#include <stdio.h>

#include "interface/application_exit_cleanup.hpp"
#include "interface/interface_receiver.hpp"
#include "registration_thread.hpp"

void interfaceRegistrationThread()
{
	g_tid tid = g_get_tid();
	if(!g_task_register_id(G_UI_REGISTRATION_THREAD_IDENTIFIER))
	{
		klog("failed to register task identifier for registration thread");
		return;
	}

	size_t bufferLength = sizeof(g_message_header) + sizeof(g_ui_initialize_request);
	uint8_t* buffer = new uint8_t[bufferLength];

	while(true)
	{
		g_message_receive_status stat = g_receive_message(buffer, bufferLength);

		if(stat == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			g_message_header* request_message = (g_message_header*) buffer;
			g_ui_initialize_request* request = (g_ui_initialize_request*) G_MESSAGE_CONTENT(buffer);

			auto receiver = new interface_receiver_t();
			g_tid receiverTid = g_create_thread_d((void*) &interfaceReceiverThread, receiver);
			g_create_thread_d((void*) &interfaceApplicationExitCleanupThread, new application_exit_cleanup_handler_t(g_get_pid_for_tid(request_message->sender), receiver));

			g_ui_initialize_response response;
			response.header.id = G_UI_PROTOCOL_INITIALIZATION;
			response.status = G_UI_PROTOCOL_SUCCESS;
			response.window_server_delegate_thread = receiverTid;
			g_send_message_t(request_message->sender, &response, sizeof(g_ui_initialize_response), request_message->transaction);
		}
	}
}
