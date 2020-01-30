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
#include <ghostuser/utils/local.hpp>
#include <ghostuser/ui/interface_specification.hpp>
#include <ghostuser/ui/titled_component.hpp>
#include <ghostuser/ui/ui.hpp>
#include <ghostuser/utils/value_placer.hpp>
#include <ghostuser/utils/local.hpp>
#include <stdio.h>

/**
 *
 */
bool g_titled_component::setTitle(std::string title) {

	if (!g_ui_initialized) {
		return 0;
	}

	// send initialization request
	g_message_transaction tx = g_get_message_tx_id();

	g_local<g_ui_component_set_title_request> request(new g_ui_component_set_title_request());
	request()->header.id = G_UI_PROTOCOL_SET_TITLE;
	request()->id = this->id;

	// fill text (truncate if necessary)
	const char* title_str = title.c_str();
	size_t title_len;
	if (title.length() >= G_UI_COMPONENT_TITLE_MAXIMUM) {
		title_len = G_UI_COMPONENT_TITLE_MAXIMUM;
	} else {
		title_len = title.length();
	}
	memcpy(request()->title, title.c_str(), title_len);
	request()->title[title_len] = 0;

	g_send_message_t(g_ui_delegate_tid, request(), sizeof(g_ui_component_set_title_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_set_title_response);
	uint8_t buffer[bufferSize];

	if (g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ui_component_set_title_response* response = (g_ui_component_set_title_response*) G_MESSAGE_CONTENT(buffer);
		if (response->status == G_UI_PROTOCOL_SUCCESS) {
			return true;
		}
	}

	return false;

}
/**
 *
 */
std::string g_titled_component::getTitle() {

	if (!g_ui_initialized) {
		return 0;
	}

	// send initialization request
	g_message_transaction tx = g_get_message_tx_id();

	g_ui_component_get_title_request request;
	request.header.id = G_UI_PROTOCOL_GET_TITLE;
	request.id = this->id;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_get_title_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_get_title_response);
	g_local<uint8_t> buffer(new uint8_t[bufferSize]);

	if (g_receive_message_t(buffer(), bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL) {
		g_ui_component_get_title_response* response = (g_ui_component_get_title_response*) G_MESSAGE_CONTENT(buffer());
		if (response->status == G_UI_PROTOCOL_SUCCESS) {
			return std::string(response->title);
		}
	}

	return std::string();

}
