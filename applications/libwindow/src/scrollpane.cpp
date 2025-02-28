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

#include "libwindow/scrollpane.hpp"

g_scrollpane* g_scrollpane::create()
{
	return createComponent<g_scrollpane, G_UI_COMPONENT_TYPE_SCROLLPANE>();
}


bool g_scrollpane::setContent(g_component* content)
{
	if(!g_ui_initialized)
		return false;

	g_message_transaction tx = g_get_message_tx_id();

	g_ui_scrollpane_set_content request;
	request.header.id = G_UI_PROTOCOL_SCROLLPANE_SET_CONTENT;
	request.scrollpane = this->id;
	request.content = content->getId();
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(request), tx);
	g_yield_t(g_ui_delegate_tid);

	size_t buflen = sizeof(g_message_header) + sizeof(g_ui_simple_response);
	uint8_t buffer[buflen];
	if(g_receive_message_t(buffer, buflen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_ui_simple_response*) G_MESSAGE_CONTENT(buffer);
		return response->status == G_UI_PROTOCOL_SUCCESS;
	}
	return false;
}

bool g_scrollpane::setFixed(bool width, bool height)
{
	if(!g_ui_initialized)
		return false;

	g_message_transaction tx = g_get_message_tx_id();

	g_ui_scrollpane_set_fixed request;
	request.header.id = G_UI_PROTOCOL_SCROLLPANE_SET_FIXED;
	request.scrollpane = this->id;
	request.width = width;
	request.height = height;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(request), tx);
	g_yield_t(g_ui_delegate_tid);

	size_t buflen = sizeof(g_message_header) + sizeof(g_ui_simple_response);
	uint8_t buffer[buflen];
	if(g_receive_message_t(buffer, buflen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_ui_simple_response*) G_MESSAGE_CONTENT(buffer);
		return response->status == G_UI_PROTOCOL_SUCCESS;
	}
	return false;
}
