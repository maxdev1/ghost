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
	if(!platformRegisterTaskIdentifier(G_UI_REGISTRY_NAME))
	{
		platformLog("failed to register as \"%s\"", G_UI_REGISTRY_NAME);
		return;
	}

	size_t buflen = SYS_MESSAGE_HEADER_SIZE + sizeof(g_ui_initialize_request);
	uint8_t buf[buflen];

	SYS_TID_T receiverTid = platformCreateThread((void*) &interfaceReceiverThread);
	while(true)
	{
		if(platformReceiveMessage(buf, buflen, SYS_TX_NONE) == SYS_MESSAGE_RECEIVE_SUCCESS)
		{
			auto body = (g_ui_initialize_request*) SYS_MESSAGE_CONTENT(buf);

			process_registry_t::bind(platformGetPidForTid(body->event_dispatcher), body->event_dispatcher);

			platformCreateThreadWithData((void*) &interfaceApplicationExitCleanupThread,
			                new application_exit_cleanup_handler_t(platformGetPidForTid(SYS_MESSAGE_SENDER(buf))));

			g_ui_initialize_response response;
			response.header.id = G_UI_PROTOCOL_INITIALIZATION;
			response.status = G_UI_PROTOCOL_SUCCESS;
			response.window_server_delegate = receiverTid;
			platformSendMessage(SYS_MESSAGE_SENDER(buf), &response, sizeof(g_ui_initialize_response),
			                 SYS_MESSAGE_TRANSACTION(buf));
		}
	}
}
