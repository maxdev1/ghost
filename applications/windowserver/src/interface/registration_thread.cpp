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

#include "interface/application_exit_cleanup.hpp"
#include "interface/interface_receiver.hpp"
#include "registration_thread.hpp"

#include "process_registry.hpp"

void interfaceRegistrationThread()
{
	if(!g_task_register_name(G_UI_REGISTRY_NAME))
	{
		klog("failed to register as \"%s\"", G_UI_REGISTRY_NAME);
		return;
	}

	size_t buflen = sizeof(g_message_header) + sizeof(g_ui_initialize_request);
	uint8_t buf[buflen];

	g_tid receiverTid = g_create_task((void*) &interfaceReceiverThread);
	while(true)
	{
		g_message_receive_status stat = g_receive_message(buf, buflen);

		if(stat == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			auto header = (g_message_header*) buf;
			auto body = (g_ui_initialize_request*) G_MESSAGE_CONTENT(buf);

			process_registry_t::bind(g_get_pid_for_tid(body->event_dispatcher), body->event_dispatcher);

			g_create_task_d((void*) &interfaceApplicationExitCleanupThread,
			                new application_exit_cleanup_handler_t(g_get_pid_for_tid(header->sender)));

			g_ui_initialize_response response;
			response.header.id = G_UI_PROTOCOL_INITIALIZATION;
			response.status = G_UI_PROTOCOL_SUCCESS;
			response.window_server_delegate = receiverTid;
			g_send_message_t(header->sender, &response, sizeof(g_ui_initialize_response),
			                 header->transaction);
		}
	}
}
