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
#include <ghost/utils/local.hpp>
#include <ghostuser/ui/interface_specification.hpp>
#include <ghostuser/ui/titled_component.hpp>
#include <ghostuser/ui/ui.hpp>
#include <ghostuser/utils/value_placer.hpp>

/**
 *
 */
bool g_titled_component::setTitle(std::string title) {

	if (!g_ui_ready) {
		return false;
	}

	// write request
	uint32_t title_len = title.length() + 1;
	uint32_t request_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_SET_TITLE_REQUEST_LENGTH + title_len;
	g_local<uint8_t> request(new uint8_t[request_length]);

	g_value_placer request_writer(request());
	request_writer.put(G_UI_PROTOCOL_SET_TITLE);
	request_writer.put(id);
	request_writer.put(title_len);
	request_writer.put((uint8_t*) title.c_str(), title_len);
	g_ui_transaction_id transaction = g_ui::send(request(), request_length);

	// wait for response
	uint32_t response_length;
	uint8_t* response;
	if (g_ui::receive(transaction, &response, &response_length)) {
		g_local<uint8_t> response_deleter(response);

		g_value_placer response_reader(response);
		g_ui_protocol_command_id command = response_reader.get<g_ui_protocol_command_id>();
		g_ui_protocol_status status = response_reader.get<g_ui_protocol_status>();

		if (command == G_UI_PROTOCOL_SET_TITLE && status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}
	return false;
}
/**
 *
 */
std::string g_titled_component::getTitle() {

	if (!g_ui_ready) {
		return "";
	}

	// write request
	uint32_t request_length = G_UI_PROTOCOL_HEADER_LENGTH + G_UI_PROTOCOL_GET_TITLE_REQUEST_LENGTH;
	g_local<uint8_t> request(new uint8_t[request_length]);

	g_value_placer request_writer(request());
	request_writer.put(G_UI_PROTOCOL_GET_TITLE);
	request_writer.put(id);
	g_ui_transaction_id transaction = g_ui::send(request(), request_length);

	// wait for response
	uint32_t response_length;
	uint8_t* response;
	if (g_ui::receive(transaction, &response, &response_length)) {
		g_local<uint8_t> response_deleter(response);

		g_value_placer response_reader(response);
		g_ui_protocol_command_id command = response_reader.get<g_ui_protocol_command_id>();
		g_ui_protocol_status status = response_reader.get<g_ui_protocol_status>();

		uint32_t title_length = response_reader.get<uint32_t>();
		g_local<char> title(new char[title_length]);
		response_reader.get((uint8_t*) title(), title_length);

		if (command == G_UI_PROTOCOL_GET_TITLE && status == G_UI_PROTOCOL_SUCCESS) {
			return std::string(title());
		}
	}
	return "";
}
