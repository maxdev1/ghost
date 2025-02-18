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

#include <utility>

#include "libwindow/focusable_component.hpp"
#include "libwindow/properties.hpp"
#include "libwindow/listener/focus_listener.hpp"

bool g_focusable_component::isFocused()
{
	uint32_t out;
	getNumericProperty(G_UI_PROPERTY_FOCUSED, &out);
	return out == 1;
}

bool g_focusable_component::setFocused(bool focused)
{
	if(!g_ui_initialized)
		return false;

	g_message_transaction tx = g_get_message_tx_id();

	g_ui_component_focus_request request;
	request.header.id = G_UI_PROTOCOL_FOCUS;
	request.id = this->id;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_focus_request), tx);

	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_focus_response);
	uint8_t buffer[bufferSize];

	bool success = false;
	if(g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_ui_component_focus_response*) G_MESSAGE_CONTENT(buffer);
		success = (response->status == G_UI_PROTOCOL_SUCCESS);
	}

	return success;
}

void g_focusable_component::addFocusListener(std::function<void(bool)> func)
{
	this->addListener(G_UI_COMPONENT_EVENT_TYPE_FOCUS, new g_focus_listener_dispatcher(std::move(func)));
}
