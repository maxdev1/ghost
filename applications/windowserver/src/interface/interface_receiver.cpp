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

#include <libwindow/interface.hpp>
#include <stdio.h>
#include <string.h>
#include <windowserver.hpp>

#include "components/button.hpp"
#include "components/canvas.hpp"
#include "components/text/text_field.hpp"
#include "components/window.hpp"
#include "components/desktop/selection.hpp"
#include "component_registry.hpp"
#include "components/scrollpane.hpp"
#include "components/image.hpp"
#include "interface/interface_receiver.hpp"

#include "layout/grid_layout_manager.hpp"
#include "layout/flex_layout_manager.hpp"

void interfaceReceiverThread()
{
	while(true)
	{
		size_t buflen = SYS_MESSAGE_HEADER_SIZE + G_UI_MAXIMUM_MESSAGE_SIZE;
		auto buf = new uint8_t[buflen];
		bool deferred = false;

		auto stat = platformReceiveMessage(buf, buflen, SYS_TX_NONE);
		if(stat == SYS_MESSAGE_RECEIVE_SUCCESS)
		{
			interfaceReceiverProcessCommand(buf);
		}
		else if(stat == SYS_MESSAGE_RECEIVE_ERROR_EXCEEDS_BUFFER)
		{
			platformLog("could not receive an incoming request, message exceeded buffer size");
		}
		else
		{
			platformLog("an unknown error occurred when trying to receive a UI request (code: %i)", stat);
		}

		if(!deferred)
			delete buf;
	}
}

void interfaceReceiverProcessCommand(void* requestMessage)
{
	auto requestUiMessage = (g_ui_message_header*) SYS_MESSAGE_CONTENT(requestMessage);
	void* responseMessage = nullptr;
	int responseLength = 0;

	if(requestUiMessage->id == G_UI_PROTOCOL_CREATE_COMPONENT)
	{
		auto createRequest = (g_ui_create_component_request*) requestUiMessage;

		component_t* component = nullptr;
		switch(createRequest->type)
		{
			case G_UI_COMPONENT_TYPE_WINDOW:
				component = new window_t();
				windowserver_t::instance()->screen->addChild(component);
				break;

			case G_UI_COMPONENT_TYPE_LABEL:
				component = new label_t();
				break;

			case G_UI_COMPONENT_TYPE_BUTTON:
				component = new button_t();
				break;

			case G_UI_COMPONENT_TYPE_TEXTFIELD:
				component = new text_field_t();
				break;

			case G_UI_COMPONENT_TYPE_CANVAS:
				component = new canvas_t(SYS_MESSAGE_SENDER(requestMessage));
				break;

			case G_UI_COMPONENT_TYPE_SELECTION:
				component = new selection_t();
				break;

			case G_UI_COMPONENT_TYPE_PANEL:
				component = new panel_t();
				break;

			case G_UI_COMPONENT_TYPE_SCROLLPANE:
				component = new scrollpane_t();
				break;

			case G_UI_COMPONENT_TYPE_IMAGE:
				component = new image_t();
				break;

			default:
				platformLog("don't know how to create a component of type %i", createRequest->type);
				break;
		}

		g_ui_component_id component_id = -1;
		if(component)
		{
			component_id = component->id;
			component_registry_t::bind(platformGetPidForTid(SYS_MESSAGE_SENDER(requestMessage)), component);
		}

		// create response message
		auto response = new g_ui_create_component_response;
		response->header.id = G_UI_PROTOCOL_CREATE_COMPONENT;
		response->id = component_id;
		response->status = (component != nullptr ? G_UI_PROTOCOL_SUCCESS : G_UI_PROTOCOL_FAIL);

		responseMessage = response;
		responseLength = sizeof(g_ui_create_component_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_ADD_COMPONENT)
	{
		auto request = (g_ui_component_add_child_request*) requestUiMessage;
		component_t* parent = component_registry_t::get(request->parent);
		component_t* child = component_registry_t::get(request->child);

		// create response message
		auto response = new g_ui_component_add_child_response;
		if(parent == nullptr || child == nullptr)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			parent->addChild(child);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_component_add_child_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_DESTROY_COMPONENT)
	{
		auto request = (g_ui_destroy_component_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		if(component)
		{
			auto parent = component->getParent();
			if(parent)
				parent->removeChild(component);
			// TODO deferred component deletion
		}
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_SET_BOUNDS)
	{
		auto request = (g_ui_component_set_bounds_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		auto response = new g_ui_simple_response;
		if(component == nullptr)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			component->setBounds(request->bounds);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_GET_BOUNDS)
	{
		auto request = (g_ui_component_get_bounds_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		auto response = new g_ui_component_get_bounds_response;
		if(component == nullptr)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			response->bounds = component->getBounds();
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_component_get_bounds_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_ADD_LISTENER)
	{
		auto request = (g_ui_component_add_listener_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		auto response = new g_ui_component_add_listener_response;
		if(component == nullptr)
		{
			platformLog("failed to attach listener since component doesn't exist");
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			component->addListener(request->event_type, request->target_thread, request->id);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_component_add_listener_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_SET_NUMERIC_PROPERTY)
	{
		auto request = (g_ui_component_set_numeric_property_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		auto response = new g_ui_component_set_numeric_property_response;
		if(component == nullptr)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			if(component->setNumericProperty(request->property, request->value))
			{
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
			else
			{
				response->status = G_UI_PROTOCOL_FAIL;
			}
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_component_set_numeric_property_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_GET_NUMERIC_PROPERTY)
	{
		auto request = (g_ui_component_get_numeric_property_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		auto response = new g_ui_component_get_numeric_property_response;
		if(component == nullptr)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			uint32_t value;
			if(component->getNumericProperty(request->property, &value))
			{
				response->value = value;
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
			else
			{
				response->status = G_UI_PROTOCOL_FAIL;
			}
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_component_get_numeric_property_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_FOCUS)
	{
		auto request = (g_ui_component_focus_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		auto response = new g_ui_component_focus_response;
		if(component == nullptr)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			windowserver_t::instance()->switchFocus(component);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_component_set_numeric_property_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_SET_STRING_PROPERTY)
	{
		auto request = (g_ui_component_set_string_property_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		auto response = new g_ui_simple_response;
		if(component == nullptr)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			component->setStringProperty(request->property, std::string(request->value));
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_GET_STRING_PROPERTY)
	{
		auto request = (g_ui_component_get_string_property_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		g_ui_component_get_string_property_response* response;
		if(component == nullptr)
		{
			response = new g_ui_component_get_string_property_response;
			response->status = G_UI_PROTOCOL_FAIL;
			responseLength = sizeof(g_ui_component_get_string_property_response);
		}
		else
		{
			std::string value;
			if(component->getStringProperty(request->property, value))
			{
				responseLength = sizeof(g_ui_component_get_string_property_response) + value.length() + 1;
				response = static_cast<g_ui_component_get_string_property_response*>(
					operator new(responseLength)
				);
				strcpy(response->value, value.c_str());
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
			else
			{
				response = new g_ui_component_get_string_property_response;
				response->status = G_UI_PROTOCOL_FAIL;
				responseLength = sizeof(g_ui_component_get_string_property_response);
			}
		}

		responseMessage = response;
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_CANVAS_BLIT)
	{
		auto request = (g_ui_component_canvas_blit_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);

		if(component)
		{
			auto canvas = dynamic_cast<canvas_t*>(component);
			if(canvas)
			{
				canvas->requestBlit(request->area);
			}
		}
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_REGISTER_DESKTOP_CANVAS)
	{
		auto request = (g_ui_register_desktop_canvas_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->canvas_id);

		auto response = new g_ui_register_desktop_canvas_response;
		if(component == nullptr)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			response->status = G_UI_PROTOCOL_SUCCESS;

			auto canvas = dynamic_cast<canvas_t*>(component);
			if(canvas)
			{
				canvas->setZIndex(1);

				screen_t* screen = windowserver_t::instance()->screen;
				screen->addChild(canvas);
				screen->addListener(G_UI_COMPONENT_EVENT_TYPE_WINDOWS, request->target_thread, canvas->id);
				canvas->setBounds(screen->getBounds());
			}
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_register_desktop_canvas_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_GET_SCREEN_DIMENSION)
	{
		auto response = new g_ui_get_screen_dimension_response;
		response->size = windowserver_t::instance()->screen->getBounds().getSize();

		responseMessage = response;
		responseLength = sizeof(g_ui_get_screen_dimension_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_FLEX_SET_ORIENTATION)
	{
		auto response = new g_ui_simple_response;
		response->status = G_UI_PROTOCOL_FAIL;

		auto request = (g_ui_flex_set_orientation_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);
		if(component)
		{
			auto flexLayoutManager = dynamic_cast<flex_layout_manager_t*>(component->getLayoutManager());
			if(flexLayoutManager)
			{
				flexLayoutManager->setHorizontal(request->horizontal);
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_FLEX_SET_COMPONENT_INFO)
	{
		auto response = new g_ui_simple_response;
		response->status = G_UI_PROTOCOL_FAIL;

		auto request = (g_ui_flex_set_component_info*) requestUiMessage;
		component_t* component = component_registry_t::get(request->parent);
		component_t* child = component_registry_t::get(request->child);
		if(component && child)
		{
			auto flexLayoutManager = dynamic_cast<flex_layout_manager_t*>(component->getLayoutManager());
			if(flexLayoutManager)
			{
				flexLayoutManager->setLayoutInfo(child, request->grow, request->shrink, request->basis);
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_LAYOUT_SET_PADDING)
	{
		auto response = new g_ui_simple_response;
		response->status = G_UI_PROTOCOL_FAIL;

		auto request = (g_ui_layout_set_padding*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);
		if(component)
		{
			auto layoutManager = component->getLayoutManager();
			if(layoutManager)
			{
				layoutManager->setPadding(request->insets);
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_SCROLLPANE_SET_CONTENT)
	{
		auto response = new g_ui_simple_response;
		response->status = G_UI_PROTOCOL_FAIL;

		auto request = (g_ui_scrollpane_set_content*) requestUiMessage;
		scrollpane_t* scrollpane = dynamic_cast<scrollpane_t*>(component_registry_t::get(request->scrollpane));
		component_t* content = component_registry_t::get(request->content);
		if(scrollpane && content)
		{
			scrollpane->setContent(content);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_SCROLLPANE_SET_FIXED)
	{
		auto response = new g_ui_simple_response;
		response->status = G_UI_PROTOCOL_FAIL;

		auto request = (g_ui_scrollpane_set_fixed*) requestUiMessage;
		scrollpane_t* scrollpane = dynamic_cast<scrollpane_t*>(component_registry_t::get(request->scrollpane));
		if(scrollpane)
		{
			scrollpane->setFixedHeight(request->height);
			scrollpane->setFixedWidth(request->width);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_SET_PREFERRED_SIZE)
	{
		auto response = new g_ui_simple_response;
		response->status = G_UI_PROTOCOL_FAIL;

		auto request = (g_ui_component_set_size_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);
		if(component)
		{
			component->setPreferredSize(request->size);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_SET_MINIMUM_SIZE)
	{
		auto response = new g_ui_simple_response;
		response->status = G_UI_PROTOCOL_FAIL;

		auto request = (g_ui_component_set_size_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);
		if(component)
		{
			component->setMinimumSize(request->size);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}
	else if(requestUiMessage->id == G_UI_PROTOCOL_SET_MAXIMUM_SIZE)
	{
		auto response = new g_ui_simple_response;
		response->status = G_UI_PROTOCOL_FAIL;

		auto request = (g_ui_component_set_size_request*) requestUiMessage;
		component_t* component = component_registry_t::get(request->id);
		if(component)
		{
			component->setMaximumSize(request->size);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseMessage = response;
		responseLength = sizeof(g_ui_simple_response);
	}

	windowserver_t::instance()->requestUpdateLater();

	if(responseMessage)
	{
		platformSendMessage(SYS_MESSAGE_SENDER(requestMessage), responseMessage, responseLength, SYS_MESSAGE_TRANSACTION(requestMessage));
		platformYieldTo(SYS_MESSAGE_SENDER(requestMessage));
	}
}
