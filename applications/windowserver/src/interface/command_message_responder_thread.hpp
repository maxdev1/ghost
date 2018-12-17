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

#ifndef __INTERFACE_COMMAND_MESSAGE_RESPONDER_THREAD__
#define __INTERFACE_COMMAND_MESSAGE_RESPONDER_THREAD__

#include <ghostuser/tasking/thread.hpp>
#include <deque>

/**
 *
 */
typedef struct {
	g_tid target;
	uint32_t transaction;
	void* message;
	size_t length;
} command_message_response_t;

/**
 *
 */
class command_message_responder_thread_t: public g_thread {
public:
	std::deque<command_message_response_t> buffer;
	uint8_t buffer_empty = true;
	uint8_t buffer_lock = 0;

	/**
	 *
	 */
	virtual void run();

	/**
	 *
	 */
	void send_response(command_message_response_t& response);

};

#endif
