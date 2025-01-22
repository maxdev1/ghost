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
g_user_mutex buffer_empty = g_mutex_initialize();
g_user_mutex buffer_lock = g_mutex_initialize();

void interfaceResponderThread()
{
	g_task_register_id("windowserver/interface-responder");
	g_mutex_acquire(buffer_empty);
	while(true)
	{
		g_mutex_acquire(buffer_empty);

		g_mutex_acquire(buffer_lock);
		while(buffer.size() > 0)
		{
			command_message_response_t& response = buffer.back();
			g_send_message_t(response.target, response.message, response.length, response.transaction);

			delete(g_message_header*) response.message;

			buffer.pop_back();
		}
		g_mutex_release(buffer_lock);
	}
}

void interfaceResponderSend(command_message_response_t& response)
{
	g_mutex_acquire(buffer_lock);
	buffer.push_back(response);
	g_mutex_release(buffer_empty);
	g_mutex_release(buffer_lock);
}
