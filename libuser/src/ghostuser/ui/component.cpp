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
#include <ghostuser/ui/interface_specification.hpp>
#include <ghostuser/utils/value_placer.hpp>

/**
 *
 */
bool g_component::addChild(g_component* child) {

	if (!g_ui_initialized) {
		return 0;
	}

	// send initialization request
	uint32_t tx = g_ipc_next_topic();

	g_ui_component_add_child_request request;
	request.header.id = G_UI_PROTOCOL_ADD_COMPONENT;
	request.parent = this->id;
	request.child = child->id;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_add_child_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_add_child_response);
	uint8_t buffer[bufferSize];

	if (g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ui_component_add_child_response* response = (g_ui_component_add_child_response*) G_MESSAGE_CONTENT(buffer);
		if (response->status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}

	return false;

}

/**
 *
 */
bool g_component::setBounds(g_rectangle rect) {

	if (!g_ui_initialized) {
		return 0;
	}

	// send initialization request
	uint32_t tx = g_ipc_next_topic();

	g_ui_component_set_bounds_request request;
	request.header.id = G_UI_PROTOCOL_SET_BOUNDS;
	request.id = this->id;
	request.bounds = rect;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_set_bounds_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_set_bounds_response);
	uint8_t buffer[bufferSize];

	if (g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ui_component_set_bounds_response* response = (g_ui_component_set_bounds_response*) G_MESSAGE_CONTENT(buffer);
		if (response->status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}

	return false;

}

/**
 *
 */
bool g_component::setVisible(bool visible) {

	if (!g_ui_initialized) {
		return 0;
	}

	// send initialization request
	uint32_t tx = g_ipc_next_topic();

	g_ui_component_set_visible_request request;
	request.header.id = G_UI_PROTOCOL_SET_VISIBLE;
	request.id = this->id;
	request.visible = visible;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_set_visible_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_set_visible_response);
	uint8_t buffer[bufferSize];

	if (g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ui_component_set_visible_response* response = (g_ui_component_set_visible_response*) G_MESSAGE_CONTENT(buffer);
		if (response->status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}

	return false;

}
