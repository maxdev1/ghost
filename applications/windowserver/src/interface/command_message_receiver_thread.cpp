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

#include <ghostuser/ui/interface_specification.hpp>
#include <interface/command_message_receiver_thread.hpp>
#include <stdio.h>
#include <windowserver.hpp>
#include <string.h>

/**
 *
 */
void command_message_receiver_thread_t::run() {

	g_tid tid = g_get_tid();

	// create a buffer for incoming command messages
	size_t bufferLength = sizeof(g_message_header) + G_UI_MAXIMUM_MESSAGE_SIZE;
	uint8_t* buffer = new uint8_t[bufferLength];

	while (true) {
		// receive messages
		g_message_receive_status stat = g_receive_message(buffer, bufferLength);
		if (stat == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
			g_message_header* request_message = (g_message_header*) buffer;

			// add message to the event processing queue
			size_t messageTotalSize = sizeof(g_message_header) + request_message->length;
			uint8_t* messageCopy = new uint8_t[messageTotalSize];
			memcpy(messageCopy, buffer, messageTotalSize);

			windowserver_t::instance()->event_processor->bufferCommandMessage(messageCopy);

		} else if (stat == G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE) {
			klog("could not receive an incoming request, message exceeded buffer size");

		} else {
			klog("an unknown error occurred when trying to receive a UI request (code: %i)", stat);

		}
	}

	delete buffer;
}

