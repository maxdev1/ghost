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
#include <ghostuser/ui/interface_specification.hpp>
#include <ghostuser/ui/canvas.hpp>

/**
 *
 */
g_canvas::~g_canvas() {
	delete graphics;
}

/**
 *
 */
g_canvas* g_canvas::create() {
	g_canvas* instance = createComponent<g_canvas, G_UI_COMPONENT_TYPE_CANVAS>();

	if (instance) {
		if (!instance->retrieveServerManagedBuffer()) {
			delete instance;
			instance = 0;
		}
	}

	return instance;
}

/**
 *
 */
bool g_canvas::retrieveServerManagedBuffer() {

	if (!g_ui_initialized) {
		return false;
	}

	// send initialization request
	uint32_t tx = g_ipc_next_topic();

	g_ui_component_get_canvas_buffer_request request;
	request.header.id = G_UI_PROTOCOL_GET_CANVAS_BUFFER_REQUEST;
	request.id = this->id;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_get_canvas_buffer_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_get_canvas_buffer_response);
	g_local<uint8_t> buffer(new uint8_t[bufferSize]);

	if (g_receive_message_t(buffer(), bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ui_component_get_canvas_buffer_response* response = (g_ui_component_get_canvas_buffer_response*) G_MESSAGE_CONTENT(buffer());

		if (response->status == G_UI_PROTOCOL_SUCCESS) {
			this->graphics = new g_graphics(false, (g_color_argb*) response->buffer, response->bufferWidth, response->bufferHeight);
			return true;
		}
	}

	return false;
}
