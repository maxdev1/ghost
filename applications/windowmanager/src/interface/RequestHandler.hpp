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

#ifndef REQUESTHANDLER_HPP_
#define REQUESTHANDLER_HPP_

#include <ghost.h>
#include <ghostuser/ui/ui.hpp>

/**
 *
 */
struct UIRegisteredProcessData {
	g_pid pid;
	g_fd output;
	g_fd input;
};

/**
 *
 */
struct UIEventDispatchData {
	g_fd output;
	uint32_t listener;
	uint8_t* data; // is deleted by dispatch thread
	uint32_t length;
};

/**
 *
 */
class RequestHandler {
public:
	static void run();

	static void send(g_fd out, g_ui_transaction_id transaction, uint8_t* data, uint32_t length);
	static void send_event(g_pid process, uint32_t listener_id, uint8_t* data, uint32_t length);

	static void handling_thread(g_message* request);
	static g_ui_protocol_status createWindow(uint32_t* out_id);
	static g_ui_protocol_status createComponent(uint32_t component_type, uint32_t* out_id);
	static g_ui_protocol_status addComponent(uint32_t parent, uint32_t child);
	static g_ui_protocol_status setTitle(uint32_t component, char* title);
	static g_ui_protocol_status getTitle(uint32_t component, std::string& out_title);
	static g_ui_protocol_status setBounds(uint32_t component, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	static g_ui_protocol_status setVisible(uint32_t component_id, bool visible);
	static g_ui_protocol_status setActionListener(g_pid process, uint32_t component, uint32_t* out_listener_id);

	static void add_process(g_pid process, g_fd output, g_fd input);
	static void remove_process(g_pid process);
	static uint32_t nextListenerId();

	static void event_dispatch_thread();
	static void event_dispatch_queue_add(const UIEventDispatchData& data);
};

#endif
