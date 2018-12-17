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

#include <interface/command_message_responder_thread.hpp>
#include <ghostuser/ui/interface_specification.hpp>
#include <stdio.h>
#include <windowserver.hpp>
#include <events/event_processor.hpp>
#include <ghost.h>

/**
 *
 */
void command_message_responder_thread_t::run() {

	event_processor_t* eventProcessor = windowserver_t::instance()->event_processor;
	while (true) {

		// wait until messages are added
		g_atomic_lock(&buffer_empty);
		g_atomic_lock(&buffer_lock);

		// process all
		while (buffer.size() > 0) {

			// get reference to response from the queue
			command_message_response_t& response = buffer.back();
			g_send_message_t(response.target, response.message, response.length, response.transaction);

			// delete message buffer
			delete (g_message_header*) response.message;

			// remove response from queue
			buffer.pop_back();
		}

		buffer_lock = 0;
	}
}

/**
 *
 */
void command_message_responder_thread_t::send_response(command_message_response_t& response) {
	g_atomic_lock(&buffer_lock);
	buffer.push_back(response);
	buffer_empty = false;
	buffer_lock = 0;
}

