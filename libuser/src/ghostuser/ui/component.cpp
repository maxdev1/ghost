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

	if (!g_ui_ready) {
		return 0;
	}

	// write request
	uint32_t request_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_ADD_COMPONENT_REQUEST_LENGTH;
	uint8_t request[request_length];
	g_value_placer request_builder(request);
	request_builder.put(G_UI_PROTOCOL_ADD_COMPONENT);
	request_builder.put(this->id);
	request_builder.put(child->id);
	g_ui_transaction_id transaction = g_ui::send(request, request_length);

	// wait for response
	uint32_t response_length;
	uint8_t* response;
	if (g_ui::receive(transaction, &response, &response_length)) {
		g_local<uint8_t> response_deleter(response);

		g_value_placer resp(response);
		g_ui_protocol_command_id command = resp.get<g_ui_protocol_command_id>();
		g_ui_protocol_status status = resp.get<g_ui_protocol_status>();

		if (command == G_UI_PROTOCOL_ADD_COMPONENT && status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}

	return false;

}

/**
 *
 */
bool g_component::setBounds(g_rectangle rect) {

	if (!g_ui_ready) {
		return false;
	}

	// write request
	uint32_t request_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_SET_BOUNDS_REQUEST_LENGTH;
	uint8_t request[request_length];
	g_value_placer reqp(request);
	reqp.put(G_UI_PROTOCOL_SET_BOUNDS);
	reqp.put(this->id);
	reqp.put(rect.x);
	reqp.put(rect.y);
	reqp.put(rect.width);
	reqp.put(rect.height);
	g_ui_transaction_id transaction = g_ui::send(request, request_length);

	// wait for response
	uint32_t response_length;
	uint8_t* response;
	if (g_ui::receive(transaction, &response, &response_length)) {
		g_local<uint8_t> response_deleter(response);

		g_value_placer resp(response);
		g_ui_protocol_command_id command = resp.get<g_ui_protocol_command_id>();
		g_ui_protocol_status status = resp.get<g_ui_protocol_status>();

		if (command == G_UI_PROTOCOL_SET_BOUNDS && status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}
	return false;
}

/**
 *
 */
bool g_component::setVisible(bool visible) {

	if (!g_ui_ready) {
		return false;
	}

	// write request
	uint32_t request_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_SET_VISIBLE_REQUEST_LENGTH;
	uint8_t request[request_length];
	g_value_placer request_putter(request);
	request_putter.put<g_ui_protocol_command_id>(G_UI_PROTOCOL_SET_VISIBLE);
	request_putter.put<uint32_t>(this->id);
	request_putter.put<uint8_t>(visible);
	g_ui_transaction_id transaction = g_ui::send(request, request_length);

	// wait for response
	uint32_t response_length;
	uint8_t* response;
	if (g_ui::receive(transaction, &response, &response_length)) {
		g_local<uint8_t> response_deleter(response);

		g_value_placer response_getter(response);
		g_ui_protocol_command_id command = response_getter.get<g_ui_protocol_command_id>();
		g_ui_protocol_status status = response_getter.get<g_ui_protocol_status>();

		if (command == G_UI_PROTOCOL_SET_VISIBLE && status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}
	return false;
}
