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

#include <stdio.h>

#include "libwindow/canvas.hpp"
#include "libwindow/component.hpp"
#include "libwindow/ui.hpp"
#include "libwindow/platform/platform.hpp"

/**
 * Global ready indicator
 */
bool g_ui_initialized = false;

/**
 * ID of the window server interface receiver task
 */
SYS_TID_T g_ui_delegate_tid = SYS_TID_NONE;

/**
 * Our local event dispatcher task ID
 */
SYS_TID_T g_ui_event_dispatcher_tid = SYS_TID_NONE;

/**
 * Opens a connection to the window server.
 */
g_ui_open_status g_ui::open()
{
	// check if already open
	if(g_ui_initialized)
	{
		return G_UI_OPEN_STATUS_EXISTING;
	}

	// get window managers id
	SYS_TID_T windowserverRegistryTask = platformAwaitUiRegistry();
	if(windowserverRegistryTask == SYS_TID_NONE)
	{
		platformLog("failed to retrieve task id of window server with identifier '%s'", G_UI_REGISTRY_NAME);
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// start event dispatcher
	g_ui_event_dispatcher_tid = platformCreateThread((void*) &eventDispatchThread);

	// send initialization request
	SYS_TX_T init_tx = platformCreateMessageTransaction();

	g_ui_initialize_request request;
	request.header.id = G_UI_PROTOCOL_INITIALIZATION;
	request.event_dispatcher = g_ui_event_dispatcher_tid;
	platformSendMessage(windowserverRegistryTask, &request, sizeof(g_ui_initialize_request), init_tx);
	platformYieldTo(g_ui_delegate_tid);

	// receive initialization response
	size_t buflen = SYS_MESSAGE_HEADER_SIZE + sizeof(g_ui_initialize_response);
	uint8_t* buf[buflen];
	if(platformReceiveMessage(buf, buflen, init_tx) != SYS_MESSAGE_RECEIVE_SUCCESS)
	{
		platformLog("failed to communicate with the window server");
		return G_UI_OPEN_STATUS_COMMUNICATION_FAILED;
	}

	// check response
	auto response = (g_ui_initialize_response*) SYS_MESSAGE_CONTENT(buf);
	if(response->status != G_UI_PROTOCOL_SUCCESS)
	{
		platformLog("failed to open UI");
		return G_UI_OPEN_STATUS_FAILED;
	}

	// mark UI as ready
	g_ui_initialized = true;
	g_ui_delegate_tid = response->window_server_delegate;
	return G_UI_OPEN_STATUS_SUCCESSFUL;
}

/**
 *
 */
bool g_ui::addListener(g_ui_component_id id, g_ui_component_event_type eventType)
{
	SYS_TX_T tx = platformCreateMessageTransaction();

	g_ui_component_add_listener_request request;
	request.header.id = G_UI_PROTOCOL_ADD_LISTENER;
	request.id = id;
	request.target_thread = g_ui_event_dispatcher_tid;
	request.event_type = eventType;
	platformSendMessage(g_ui_delegate_tid, &request, sizeof(g_ui_component_add_listener_request), tx);
	platformYieldTo(g_ui_delegate_tid);

	size_t bufferSize = SYS_MESSAGE_HEADER_SIZE + sizeof(g_ui_component_add_listener_response);
	uint8_t buffer[bufferSize];
	if(platformReceiveMessage(buffer, bufferSize, tx) == SYS_MESSAGE_RECEIVE_SUCCESS)
	{
		auto response = (g_ui_component_add_listener_response*) SYS_MESSAGE_CONTENT(buffer);
		return response->status == G_UI_PROTOCOL_SUCCESS;
	}
	return false;
}

/**
 *
 */
void g_ui::eventDispatchThread()
{
	size_t bufLen = G_UI_MAXIMUM_MESSAGE_SIZE;
	auto buf = new uint8_t[bufLen];

	while(true)
	{
		auto stat = platformReceiveMessage(buf, bufLen, SYS_TX_NONE);
		if(stat == SYS_MESSAGE_RECEIVE_SUCCESS)
		{
			// event message
			auto event_header = (g_ui_component_event_header*) SYS_MESSAGE_CONTENT(buf);
			g_component* component = g_component_registry::get(event_header->component_id);

			if(component == nullptr)
			{
				platformLog("event received for unknown component %i", event_header->component_id);
				continue;
			}

			component->handle(event_header);
		}
		else
		{
			platformLog("something went wrong when receiving an event, status code: %i", stat);
		}
	}

	delete buf;
}

/**
 *
 */
bool g_ui::registerDesktopCanvas(g_canvas* c)
{
	if(!g_ui_initialized)
	{
		return false;
	}

	SYS_TX_T tx = platformCreateMessageTransaction();

	// send registration request
	g_ui_register_desktop_canvas_request request;
	request.header.id = G_UI_PROTOCOL_REGISTER_DESKTOP_CANVAS;
	request.canvas_id = c->getId();
	request.target_thread = g_ui_event_dispatcher_tid;
	platformSendMessage(g_ui_delegate_tid, &request, sizeof(g_ui_register_desktop_canvas_request), tx);
	platformYieldTo(g_ui_delegate_tid);

	// read response
	size_t buflen = SYS_MESSAGE_HEADER_SIZE + sizeof(g_ui_register_desktop_canvas_response);
	uint8_t buf[buflen];

	bool success = false;
	if(platformReceiveMessage(buf, buflen, tx) == SYS_MESSAGE_RECEIVE_SUCCESS)
	{
		auto response = (g_ui_register_desktop_canvas_response*) SYS_MESSAGE_CONTENT(buf);
		success = (response->status == G_UI_PROTOCOL_SUCCESS);
	}
	return success;
}

/**
 *
 */
bool g_ui::getScreenDimension(g_dimension& out)
{
	if(!g_ui_initialized)
	{
		return false;
	}

	SYS_TX_T tx = platformCreateMessageTransaction();

	// send request
	g_ui_get_screen_dimension_request request;
	request.header.id = G_UI_PROTOCOL_GET_SCREEN_DIMENSION;
	platformSendMessage(g_ui_delegate_tid, &request, sizeof(g_ui_get_screen_dimension_request), tx);
	platformYieldTo(g_ui_delegate_tid);

	// read response
	size_t bufferSize = SYS_MESSAGE_HEADER_SIZE + sizeof(g_ui_get_screen_dimension_response);
	uint8_t* buffer = new uint8_t[bufferSize];

	bool success = false;
	if(platformReceiveMessage(buffer, bufferSize, tx) == SYS_MESSAGE_RECEIVE_SUCCESS)
	{
		g_ui_get_screen_dimension_response* response = (g_ui_get_screen_dimension_response*) SYS_MESSAGE_CONTENT(buffer);
		out = response->size;
		success = true;
	}

	return success;
}
