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
#include "interface/component_registry.hpp"
#include "interface/interface_receiver.hpp"

void interfaceReceiverThread(interface_receiver_t* receiver)
{
	receiver->run();
}

void interface_receiver_t::run()
{
	// create a buffer for incoming command messages
	size_t bufferLength = sizeof(g_message_header) + G_UI_MAXIMUM_MESSAGE_SIZE;
	uint8_t* buffer = new uint8_t[bufferLength];

	for(;;)
	{
		// receive messages
		g_message_receive_status stat = g_receive_message_tmb(
				buffer, bufferLength,
				G_MESSAGE_TRANSACTION_NONE, G_MESSAGE_RECEIVE_MODE_BLOCKING,
				stop); // TODO break condition not working

		if(stat == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			g_message_header* message = (g_message_header*) buffer;

			command_message_response_t response;
			response.target = message->sender;
			response.transaction = message->transaction;
			response.message = 0;

			processCommand(message->sender, (g_ui_message_header*) G_MESSAGE_CONTENT(message), response);

			if(response.message)
				interfaceResponderSend(response);
		}
		else if(stat == G_MESSAGE_RECEIVE_STATUS_EXCEEDS_BUFFER_SIZE)
		{
			klog("could not receive an incoming request, message exceeded buffer size");
		}
		else
		{
			klog("an unknown error occurred when trying to receive a UI request (code: %i)", stat);
		}
	}

	delete buffer;
}

void interface_receiver_t::processCommand(g_tid senderTid, g_ui_message_header* requestIn,
                                          command_message_response_t& responseOut)
{
	if(requestIn->id == G_UI_PROTOCOL_CREATE_COMPONENT)
	{
		g_ui_create_component_request* create_request = (g_ui_create_component_request*) requestIn;

		component_t* component = 0;
		switch(create_request->type)
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
				component = new canvas_t(senderTid);
				break;

			case G_UI_COMPONENT_TYPE_SELECTION:
				component = new selection_t();
				break;


			default:
				klog("don't know how to create a component of type %i", create_request->type);
				break;
		}

		g_ui_component_id component_id = -1;
		if(component)
		{
			component_id = component->id;
			component_registry_t::bind(g_get_pid_for_tid(senderTid), component);
		}

		// create response message
		g_ui_create_component_response* response = new g_ui_create_component_response;
		response->header.id = G_UI_PROTOCOL_CREATE_COMPONENT;
		response->id = component_id;
		response->status = (component != 0 ? G_UI_PROTOCOL_SUCCESS : G_UI_PROTOCOL_FAIL);

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_create_component_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_ADD_COMPONENT)
	{
		g_ui_component_add_child_request* request = (g_ui_component_add_child_request*) requestIn;
		component_t* parent = component_registry_t::get(request->parent);
		component_t* child = component_registry_t::get(request->child);

		// create response message
		g_ui_component_add_child_response* response = new g_ui_component_add_child_response;
		if(parent == 0 || child == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
			klog("could not add %i (%x) to %i (%x)", request->child, child, request->parent, parent);
		}
		else
		{
			parent->addChild(child);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_add_child_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_SET_BOUNDS)
	{
		g_ui_component_set_bounds_request* request = (g_ui_component_set_bounds_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_bounds_response* response = new g_ui_component_set_bounds_response;
		if(component == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			component->setBounds(request->bounds);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_set_bounds_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_GET_BOUNDS)
	{
		g_ui_component_get_bounds_request* request = (g_ui_component_get_bounds_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_get_bounds_response* response = new g_ui_component_get_bounds_response;
		if(component == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			response->bounds = component->getBounds();
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_get_bounds_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_SET_VISIBLE)
	{
		g_ui_component_set_visible_request* request = (g_ui_component_set_visible_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_visible_response* response = new g_ui_component_set_visible_response;
		if(component == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			component->setVisible(request->visible);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_set_visible_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_SET_LISTENER)
	{
		g_ui_component_set_listener_request* request = (g_ui_component_set_listener_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_listener_response* response = new g_ui_component_set_listener_response;
		if(component == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			component->setListener(request->event_type, request->target_thread, request->id);
			response->status = G_UI_PROTOCOL_SUCCESS;
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_set_listener_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_SET_NUMERIC_PROPERTY)
	{
		g_ui_component_set_numeric_property_request* request = (g_ui_component_set_numeric_property_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_numeric_property_response* response = new g_ui_component_set_numeric_property_response;
		if(component == 0)
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

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_set_numeric_property_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_GET_NUMERIC_PROPERTY)
	{
		g_ui_component_get_numeric_property_request* request = (g_ui_component_get_numeric_property_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_get_numeric_property_response* response = new g_ui_component_get_numeric_property_response;
		if(component == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			uint32_t out;
			if(component->getNumericProperty(request->property, &out))
			{
				response->value = out;
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
			else
			{
				response->status = G_UI_PROTOCOL_FAIL;
			}
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_get_numeric_property_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_SET_TITLE)
	{
		g_ui_component_set_title_request* request = (g_ui_component_set_title_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_set_title_response* response = new g_ui_component_set_title_response;
		if(component == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			titled_component_t* titled_component = dynamic_cast<titled_component_t*>(component);
			if(titled_component == 0)
			{
				response->status = G_UI_PROTOCOL_FAIL;
			}
			else
			{
				titled_component->setTitle(request->title);
				response->status = G_UI_PROTOCOL_SUCCESS;
			}
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_set_title_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_GET_TITLE)
	{
		g_ui_component_get_title_request* request = (g_ui_component_get_title_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		// create response message
		g_ui_component_get_title_response* response = new g_ui_component_get_title_response;
		if(component == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			titled_component_t* titled_component = dynamic_cast<titled_component_t*>(component);
			if(titled_component == 0)
			{
				response->status = G_UI_PROTOCOL_FAIL;
			}
			else
			{
				std::string title = titled_component->getTitle();

				// fill text (truncate if necessary)
				const char* title_str = title.c_str();
				size_t title_len;
				if(title.length() >= G_UI_COMPONENT_TITLE_MAXIMUM)
				{
					title_len = G_UI_COMPONENT_TITLE_MAXIMUM;
				}
				else
				{
					title_len = title.length();
				}
				memcpy(response->title, title.c_str(), title_len);
				response->title[title_len] = 0;

				response->status = G_UI_PROTOCOL_SUCCESS;
			}
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_component_get_title_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_CANVAS_BLIT)
	{
		g_ui_component_canvas_blit_request* request = (g_ui_component_canvas_blit_request*) requestIn;
		component_t* component = component_registry_t::get(request->id);

		if(component)
		{
			canvas_t* canvas = (canvas_t*) component;
			canvas->requestBlit(request->area);
		}
	}
	else if(requestIn->id == G_UI_PROTOCOL_REGISTER_DESKTOP_CANVAS)
	{
		g_ui_register_desktop_canvas_request* request = (g_ui_register_desktop_canvas_request*) requestIn;
		component_t* component = component_registry_t::get(request->canvas_id);

		// create response message
		g_ui_register_desktop_canvas_response* response = new g_ui_register_desktop_canvas_response;
		if(component == 0)
		{
			response->status = G_UI_PROTOCOL_FAIL;
		}
		else
		{
			response->status = G_UI_PROTOCOL_SUCCESS;

			canvas_t* canvas = (canvas_t*) component;
			canvas->setZIndex(1);

			screen_t* screen = windowserver_t::instance()->screen;
			screen->addChild(canvas);
			canvas->setBounds(screen->getBounds());
		}

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_register_desktop_canvas_response);
	}
	else if(requestIn->id == G_UI_PROTOCOL_GET_SCREEN_DIMENSION)
	{
		g_ui_get_screen_dimension_request* request = (g_ui_get_screen_dimension_request*) requestIn;

		g_ui_get_screen_dimension_response* response = new g_ui_get_screen_dimension_response;
		response->size = windowserver_t::instance()->screen->getBounds().getSize();

		responseOut.message = response;
		responseOut.length = sizeof(g_ui_get_screen_dimension_response);
	}
}
