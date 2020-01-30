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

#include <ghost.h>
#include <ghostuser/ui/component.hpp>
#include <ghostuser/ui/listener.hpp>
#include <ghostuser/ui/ui.hpp>
#include <ghostuser/ui/canvas.hpp>
#include <ghostuser/ui/action_component.hpp>
#include <ghostuser/ui/action_listener.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/utils/value_placer.hpp>
#include <ghostuser/utils/local.hpp>
#include <map>
#include <deque>
#include <stdio.h>

/**
 * Global ready indicator
 */
bool g_ui_initialized = false;

/**
 *
 */
g_tid g_ui_delegate_tid = -1;
g_tid g_ui_event_dispatcher_tid = -1;

/**
 * Opens a connection to the window server.
 */
g_ui_open_status g_ui::open() {

	// check if already open
	if (g_ui_initialized) {
		return G_UI_OPEN_STATUS_EXISTING;
	}

	// get window managers id
	g_tid window_mgr = g_task_get_id(G_UI_REGISTRATION_THREAD_IDENTIFIER);
	if (window_mgr == -1) {
		klog("failed to retrieve task id of window server with identifier '%s'", G_UI_REGISTRATION_THREAD_IDENTIFIER);
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// start event dispatcher
	g_ui_event_dispatcher_tid = g_create_thread((void*) &event_dispatch_thread);

	// send initialization request
	g_message_transaction init_tx = g_get_message_tx_id();

	g_ui_initialize_request request;
	request.header.id = G_UI_PROTOCOL_INITIALIZATION;
	g_send_message_t(window_mgr, &request, sizeof(g_ui_initialize_request), init_tx);

	// receive initialization response
	uint32_t response_buffer_size = sizeof(g_message_header) + sizeof(g_ui_initialize_response);
	g_local<uint8_t> response_buffer(new uint8_t[response_buffer_size]);
	if (g_receive_message_t(response_buffer(), response_buffer_size, init_tx) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		klog("failed to communicate with the window server");
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// check response
	g_ui_initialize_response* response = (g_ui_initialize_response*) G_MESSAGE_CONTENT(response_buffer());
	if (response->status != G_UI_PROTOCOL_SUCCESS) {
		klog("failed to open UI");
		return G_UI_OPEN_STATUS_FAILED;
	}

	// mark UI as ready
	g_ui_initialized = true;
	g_ui_delegate_tid = response->window_server_delegate_thread;
	return G_UI_OPEN_STATUS_SUCCESSFUL;
}

/**
 *
 */
void g_ui::event_dispatch_thread() {

	size_t buffer_size = G_UI_MAXIMUM_MESSAGE_SIZE;
	g_local<uint8_t> buffer(new uint8_t[buffer_size]);

	while (true) {
		auto stat = g_receive_message(buffer(), buffer_size);
		if (stat == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {

			// event message
			g_ui_component_event_header* event_header = (g_ui_component_event_header*) G_MESSAGE_CONTENT(buffer());
			g_component* component = g_component_registry::get(event_header->component_id);

			if (component == 0) {
				klog("event received for unknown component %i", event_header->component_id);
				continue;
			}

			// tell the component delegate to handle the event
			component->handle(event_header);

		} else {
			klog("something went wrong when receiving an event, status code: %i", stat);
		}
	}
}

/**
 *
 */
bool g_ui::register_desktop_canvas(g_canvas* c) {

	if (!g_ui_initialized) {
		return false;
	}

	g_message_transaction tx = g_get_message_tx_id();

	// send registration request
	g_ui_register_desktop_canvas_request request;
	request.header.id = G_UI_PROTOCOL_REGISTER_DESKTOP_CANVAS;
	request.canvas_id = c->getId();
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_register_desktop_canvas_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_register_desktop_canvas_response);
	g_local<uint8_t> buffer(new uint8_t[bufferSize]);

	if (g_receive_message_t(buffer(), bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ui_register_desktop_canvas_response* response = (g_ui_register_desktop_canvas_response*) G_MESSAGE_CONTENT(buffer());

		if (response->status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}

	return false;
}

/**
 *
 */
bool g_ui::get_screen_dimension(g_dimension* out) {

	if (!g_ui_initialized) {
		return false;
	}

	g_message_transaction tx = g_get_message_tx_id();

	// send request
	g_ui_get_screen_dimension_request request;
	request.header.id = G_UI_PROTOCOL_GET_SCREEN_DIMENSION;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_get_screen_dimension_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_get_screen_dimension_response);
	g_local<uint8_t> buffer(new uint8_t[bufferSize]);

	if (g_receive_message_t(buffer(), bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ui_get_screen_dimension_response* response = (g_ui_get_screen_dimension_response*) G_MESSAGE_CONTENT(buffer());
		*out = response->size;
		return true;
	}

	return false;
}
