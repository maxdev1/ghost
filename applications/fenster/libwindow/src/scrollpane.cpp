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

	SYS_TX_T tx = platformCreateMessageTransaction();

	g_ui_scrollpane_set_content request;
	request.header.id = G_UI_PROTOCOL_SCROLLPANE_SET_CONTENT;
	request.scrollpane = this->id;
	request.content = content->getId();
	platformSendMessage(g_ui_delegate_tid, &request, sizeof(request), tx);
	platformYieldTo(g_ui_delegate_tid);

	size_t buflen = SYS_MESSAGE_HEADER_SIZE + sizeof(g_ui_simple_response);
	uint8_t buffer[buflen];
	if(platformReceiveMessage(buffer, buflen, tx) == SYS_MESSAGE_RECEIVE_SUCCESS)
	{
		auto response = (g_ui_simple_response*) SYS_MESSAGE_CONTENT(buffer);
		return response->status == G_UI_PROTOCOL_SUCCESS;
	}
	return false;
}

bool g_scrollpane::setFixed(bool width, bool height)
{
	if(!g_ui_initialized)
		return false;

	SYS_TX_T tx = platformCreateMessageTransaction();

	g_ui_scrollpane_set_fixed request;
	request.header.id = G_UI_PROTOCOL_SCROLLPANE_SET_FIXED;
	request.scrollpane = this->id;
	request.width = width;
	request.height = height;
	platformSendMessage(g_ui_delegate_tid, &request, sizeof(request), tx);
	platformYieldTo(g_ui_delegate_tid);

	size_t buflen = SYS_MESSAGE_HEADER_SIZE + sizeof(g_ui_simple_response);
	uint8_t buffer[buflen];
	if(platformReceiveMessage(buffer, buflen, tx) == SYS_MESSAGE_RECEIVE_SUCCESS)
	{
		auto response = (g_ui_simple_response*) SYS_MESSAGE_CONTENT(buffer);
		return response->status == G_UI_PROTOCOL_SUCCESS;
	}
	return false;
}
