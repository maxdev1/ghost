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

#include "events/event_processor.hpp"
#include "interface/interface_responder.hpp"

std::deque<command_message_response_t> buffer;
g_atom buffer_empty = g_atomic_initialize();
g_atom buffer_lock = g_atomic_initialize();

void interfaceResponderThread()
{
	g_task_register_id("windowserver/interface-responder");
	g_atomic_lock(buffer_empty);
	while(true)
	{
		g_atomic_lock(buffer_empty);

		g_atomic_lock(buffer_lock);
		while(buffer.size() > 0)
		{
			command_message_response_t& response = buffer.back();
			g_send_message_t(response.target, response.message, response.length, response.transaction);

			delete(g_message_header*) response.message;

			buffer.pop_back();
		}
		g_atomic_unlock(buffer_lock);
	}
}

void interfaceResponderSend(command_message_response_t& response)
{
	g_atomic_lock(buffer_lock);
	buffer.push_back(response);
	g_atomic_unlock(buffer_empty);
	g_atomic_unlock(buffer_lock);
}
