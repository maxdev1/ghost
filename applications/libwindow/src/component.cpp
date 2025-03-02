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

#include "libwindow/component.hpp"
#include "libwindow/properties.hpp"

g_component::~g_component()
{
	destroy();
}

void g_component::destroy()
{
	if(destroyed)
		return;
	destroyed = true;

	g_ui_destroy_component_request request;
	request.header.id = G_UI_PROTOCOL_DESTROY_COMPONENT;
	request.id = this->id;
	g_send_message(g_ui_delegate_tid, &request, sizeof(g_ui_destroy_component_request));
	g_yield_t(g_ui_delegate_tid);
}

bool g_component::addChild(g_component* child)
{
	if(!g_ui_initialized)
		return false;

	// send initialization request
	g_message_transaction tx = g_get_message_tx_id();

	g_ui_component_add_child_request request;
	request.header.id = G_UI_PROTOCOL_ADD_COMPONENT;
	request.parent = this->id;
	request.child = child->id;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_add_child_request), tx);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_add_child_response);
	uint8_t buffer[bufferSize];

	if(g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_ui_component_add_child_response*) G_MESSAGE_CONTENT(buffer);
		if(response->status == G_UI_PROTOCOL_SUCCESS)
		{
			return true;
		}
	}

	return false;
}

bool g_component::setBounds(const g_rectangle& rect)
{
	if(!g_ui_initialized)
		return false;

	// send initialization request
	g_message_transaction tx = g_get_message_tx_id();

	g_ui_component_set_bounds_request request;
	request.header.id = G_UI_PROTOCOL_SET_BOUNDS;
	request.id = this->id;
	request.bounds = rect;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_set_bounds_request), tx);
	g_yield_t(g_ui_delegate_tid);

	// read response
	size_t buflen = sizeof(g_message_header) + sizeof(g_ui_simple_response);
	uint8_t buffer[buflen];
	if(g_receive_message_t(buffer, buflen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_ui_simple_response*) G_MESSAGE_CONTENT(buffer);
		return response->status == G_UI_PROTOCOL_SUCCESS;
	}
	return false;
}

g_rectangle g_component::getBounds()
{
	if(!g_ui_initialized)
		return g_rectangle();

	// send initialization request
	g_message_transaction tx = g_get_message_tx_id();

	g_ui_component_get_bounds_request request;
	request.header.id = G_UI_PROTOCOL_GET_BOUNDS;
	request.id = this->id;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_get_bounds_request), tx);
	g_yield_t(g_ui_delegate_tid);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_get_bounds_response);
	uint8_t buffer[bufferSize];

	g_rectangle result;
	if(g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_ui_component_get_bounds_response*) G_MESSAGE_CONTENT(buffer);
		if(response->status == G_UI_PROTOCOL_SUCCESS)
			result = response->bounds;
	}

	return result;
}

bool g_component::setNumericProperty(int property, uint32_t value)
{
	if(!g_ui_initialized)
		return false;

	// send initialization request
	g_message_transaction tx = g_get_message_tx_id();

	g_ui_component_set_numeric_property_request request;
	request.header.id = G_UI_PROTOCOL_SET_NUMERIC_PROPERTY;
	request.id = this->id;
	request.property = property;
	request.value = value;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_set_numeric_property_request), tx);
	g_yield_t(g_ui_delegate_tid);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_set_numeric_property_response);
	uint8_t buffer[bufferSize];

	bool success = false;
	if(g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_ui_component_set_numeric_property_response*)
				G_MESSAGE_CONTENT(buffer);
		success = (response->status == G_UI_PROTOCOL_SUCCESS);
	}

	return success;
}

bool g_component::getNumericProperty(int property, uint32_t* out)
{
	if(!g_ui_initialized)
		return false;

	// send initialization request
	g_message_transaction tx = g_get_message_tx_id();

	g_ui_component_get_numeric_property_request request;
	request.header.id = G_UI_PROTOCOL_GET_NUMERIC_PROPERTY;
	request.id = this->id;
	request.property = property;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_component_get_numeric_property_request), tx);
	g_yield_t(g_ui_delegate_tid);

	// read response
	size_t bufferSize = sizeof(g_message_header) + sizeof(g_ui_component_get_numeric_property_response);
	uint8_t buffer[bufferSize];

	bool success = false;
	if(g_receive_message_t(buffer, bufferSize, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_ui_component_get_numeric_property_response*)
				G_MESSAGE_CONTENT(buffer);

		if(response->status == G_UI_PROTOCOL_SUCCESS)
		{
			*out = response->value;
			success = true;
		}
	}

	return success;
}

bool g_component::addListener(g_ui_component_event_type eventType, g_listener* newListener)
{
	if(!g_ui_initialized)
		return false;

	if(newListener == nullptr)
		return false;

	g_mutex_acquire(listenersLock);
	listeners[eventType].push_back(newListener);
	g_mutex_release(listenersLock);

	return g_ui::addListener(this->id, eventType);
}

void g_component::handle(g_ui_component_event_header* header)
{
	auto eventType = header->type;

	g_mutex_acquire(listenersLock);
	auto it = listeners.find(eventType);
	if(it != listeners.end())
	{
		for(auto& listener: it->second)
		{
			listener->process(header);
		}
	}
	else
	{
		klog("incoming event (%i) but no one to handle", eventType);
	}
	g_mutex_release(listenersLock);
}

bool g_component::addMouseListener(g_mouse_listener* listener)
{
	return addListener(G_UI_COMPONENT_EVENT_TYPE_MOUSE, listener);
}

bool g_component::addMouseListener(g_mouse_listener_func func)
{
	return addListener(G_UI_COMPONENT_EVENT_TYPE_MOUSE, new g_mouse_listener_dispatcher(std::move(func)));
}

bool g_component::addVisibleListener(g_visible_listener* listener)
{
	return addListener(G_UI_COMPONENT_EVENT_TYPE_VISIBLE, listener);
}

bool g_component::addVisibleListener(g_visible_listener_func func)
{
	return addListener(G_UI_COMPONENT_EVENT_TYPE_VISIBLE, new g_visible_listener_dispatcher(std::move(func)));
}

bool g_component::setLayout(g_ui_layout_manager layout)
{
	return setNumericProperty(G_UI_PROPERTY_LAYOUT_MANAGER, layout);
}

bool g_component::setBackground(g_color_argb argb)
{
	return setNumericProperty(G_UI_PROPERTY_BACKGROUND, argb);
}

bool g_component::isVisible()
{
	uint32_t visible;
	if(getNumericProperty(G_UI_PROPERTY_VISIBLE, &visible))
	{
		return visible == 1;
	}
	return false;
}

bool g_component::setVisible(bool visible)
{
	return setNumericProperty(G_UI_PROPERTY_VISIBLE, visible ? 1 : 0);
}


bool g_component::isFocusable()
{
	uint32_t focusable;
	if(getNumericProperty(G_UI_PROPERTY_FOCUSABLE, &focusable))
	{
		return focusable == 1;
	}
	return false;
}

bool g_component::setFocusable(bool focusable)
{
	return setNumericProperty(G_UI_PROPERTY_FOCUSABLE, focusable ? 1 : 0);
}


bool g_component::isDispatchesFocus()
{
	uint32_t d;
	if(getNumericProperty(G_UI_PROPERTY_DISPATCHES_FOCUS, &d))
	{
		return d == 1;
	}
	return false;
}

bool g_component::setDispatchesFocus(bool d)
{
	return setNumericProperty(G_UI_PROPERTY_DISPATCHES_FOCUS, d ? 1 : 0);
}

bool g_component::setPreferredSize(const g_dimension& size)
{
	return setSize(G_UI_PROTOCOL_SET_PREFERRED_SIZE, size);
}

bool g_component::setMinimumSize(const g_dimension& size)
{
	return setSize(G_UI_PROTOCOL_SET_MINIMUM_SIZE, size);
}

bool g_component::setMaximumSize(const g_dimension& size)
{
	return setSize(G_UI_PROTOCOL_SET_MAXIMUM_SIZE, size);
}

bool g_component::setSize(g_ui_protocol_command_id command, const g_dimension& size)
{
	if(!g_ui_initialized)
		return false;

	g_message_transaction tx = g_get_message_tx_id();

	g_ui_component_set_size_request request;
	request.header.id = command;
	request.id = this->id;
	request.size = size;
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

bool g_component::setFlexOrientation(bool horizontal)
{
	if(!g_ui_initialized)
		return false;

	g_message_transaction tx = g_get_message_tx_id();

	g_ui_flex_set_orientation_request request;
	request.header.id = G_UI_PROTOCOL_FLEX_SET_ORIENTATION;
	request.id = this->id;
	request.horizontal = horizontal;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_flex_set_orientation_request), tx);
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

bool g_component::setFlexComponentInfo(g_component* child, float grow, float shrink, int basis)
{
	if(!g_ui_initialized)
		return false;

	g_message_transaction tx = g_get_message_tx_id();

	g_ui_flex_set_component_info request;
	request.header.id = G_UI_PROTOCOL_FLEX_SET_COMPONENT_INFO;
	request.parent = this->id;
	request.child = child->id;
	request.grow = grow;
	request.shrink = shrink;
	request.basis = basis;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_flex_set_component_info), tx);
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


bool g_component::setLayoutPadding(g_insets padding)
{
	if(!g_ui_initialized)
		return false;

	g_message_transaction tx = g_get_message_tx_id();

	g_ui_layout_set_padding request;
	request.header.id = G_UI_PROTOCOL_LAYOUT_SET_PADDING;
	request.id = this->id;
	request.insets = padding;
	g_send_message_t(g_ui_delegate_tid, &request, sizeof(g_ui_layout_set_padding), tx);
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


bool g_component::setFlexGap(int gap)
{
	return setNumericProperty(G_UI_PROPERTY_FLEX_GAP, gap);
}
