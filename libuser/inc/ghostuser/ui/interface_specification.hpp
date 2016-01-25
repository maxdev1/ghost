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

#ifndef GHOSTLIBRARY_UI_INTERFACESPECIFICATION
#define GHOSTLIBRARY_UI_INTERFACESPECIFICATION

#include <ghost.h>
#include <ghostuser/graphics/metrics/rectangle.hpp>

/**
 * This UI interface specification defines the messages
 * that the active window manager must understand.
 */
#define G_UI_REGISTRATION_THREAD_IDENTIFIER			"windowserver/registration"

/**
 * Size of the largest expected message
 */
#define G_UI_MAXIMUM_MESSAGE_SIZE					4096

#define G_UI_COMPONENT_TITLE_MAXIMUM				1024

/**
 * Declared in the UI unit
 */
extern bool g_ui_initialized;

/**
 * ID for a UI component
 */
typedef int32_t g_ui_component_id;

/**
 * ID for a listener
 */
typedef int32_t g_ui_listener_id;

/**
 * A protocol message always starts with the header, the message id
 */
typedef uint8_t g_ui_protocol_command_id;
const g_ui_protocol_command_id G_UI_PROTOCOL_INITIALIZATION = 1;
const g_ui_protocol_command_id G_UI_PROTOCOL_CREATE_COMPONENT = 2;
const g_ui_protocol_command_id G_UI_PROTOCOL_ADD_COMPONENT = 3;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_TITLE = 4;
const g_ui_protocol_command_id G_UI_PROTOCOL_GET_TITLE = 5;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_BOUNDS = 6;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_VISIBLE = 7;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_LISTENER = 8;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_BOOL_PROPERTY = 9;
const g_ui_protocol_command_id G_UI_PROTOCOL_GET_BOOL_PROPERTY = 10;
const g_ui_protocol_command_id G_UI_PROTOCOL_GET_CANVAS_BUFFER_REQUEST = 11;
const g_ui_protocol_command_id G_UI_PROTOCOL_GET_BOUNDS = 12;

/**
 * Common status for requests
 */
typedef uint8_t g_ui_protocol_status;
const g_ui_protocol_status G_UI_PROTOCOL_SUCCESS = 0;
const g_ui_protocol_status G_UI_PROTOCOL_FAIL = 1;

/**
 * Component types
 */
typedef uint32_t g_ui_component_type;
const g_ui_component_type G_UI_COMPONENT_TYPE_WINDOW = 0;
const g_ui_component_type G_UI_COMPONENT_TYPE_BUTTON = 1;
const g_ui_component_type G_UI_COMPONENT_TYPE_LABEL = 2;
const g_ui_component_type G_UI_COMPONENT_TYPE_TEXTFIELD = 3;
const g_ui_component_type G_UI_COMPONENT_TYPE_CANVAS = 4;

/**
 * Types of events that can be listened to
 */
typedef uint32_t g_ui_component_event_type;
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_ACTION = 0;
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_BOUNDS = 1;

/**
 *
 */
typedef struct {
	g_ui_protocol_command_id id;
}__attribute__((packed)) g_ui_message_header;

/**
 * Request to initialize interface communications. The window server creates a
 * delegate thread that is responsible for further communications and responds
 * with a <g_ui_initialize_response>.
 */
typedef struct {
	g_ui_message_header header;
}__attribute__((packed)) g_ui_initialize_request;

/**
 * Response for initializing interface communications.
 *
 * @field status
 * 		whether the initialization was successful
 * @field window_server_delegate_thread
 * 		id of the thread that is responsible for further window server communication
 */
typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
	g_tid window_server_delegate_thread;
}__attribute__((packed)) g_ui_initialize_response;

/**
 * Request sent to create a component.
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_type type;
}__attribute__((packed)) g_ui_create_component_request;

/**
 * Response when creating a component.
 */
typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
	g_ui_component_id id;
}__attribute__((packed)) g_ui_create_component_response;

/**
 * Request/response for adding a child
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id parent;
	g_ui_component_id child;
}__attribute__((packed)) g_ui_component_add_child_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
}__attribute__((packed)) g_ui_component_add_child_response;

/**
 * Request/response for setting bounds
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
	g_rectangle bounds;
}__attribute__((packed)) g_ui_component_set_bounds_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
}__attribute__((packed)) g_ui_component_set_bounds_response;

/**
 * Request/response for getting bounds
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
}__attribute__((packed)) g_ui_component_get_bounds_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
	g_rectangle bounds;
}__attribute__((packed)) g_ui_component_get_bounds_response;

/**
 * Request/response for setting components (in)visible
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
	bool visible;
}__attribute__((packed)) g_ui_component_set_visible_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
}__attribute__((packed)) g_ui_component_set_visible_response;

/**
 * Request/response for setting the title on a titled component
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
	char title[G_UI_COMPONENT_TITLE_MAXIMUM];
}__attribute__((packed)) g_ui_component_set_title_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
}__attribute__((packed)) g_ui_component_set_title_response;

/**
 * Request/response for getting the title on a titled component
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
}__attribute__((packed)) g_ui_component_get_title_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
	char title[G_UI_COMPONENT_TITLE_MAXIMUM];
}__attribute__((packed)) g_ui_component_get_title_response;

/**
 * Request/response for getting a bool property
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
	int property;
}__attribute__((packed)) g_ui_component_get_bool_property_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
	uint8_t value;
}__attribute__((packed)) g_ui_component_get_bool_property_response;

/**
 * Request/response for setting a bool property
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
	int property;
	uint8_t value;
}__attribute__((packed)) g_ui_component_set_bool_property_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
}__attribute__((packed)) g_ui_component_set_bool_property_response;

/**
 * Request/response for getting the buffer of a canvas
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
}__attribute__((packed)) g_ui_component_get_canvas_buffer_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
	uint8_t* buffer;
	uint16_t bufferWidth;
	uint16_t bufferHeight;
}__attribute__((packed)) g_ui_component_get_canvas_buffer_response;

/**
 * Event handler registration functions
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_id id;
	g_ui_component_event_type event_type;
	g_tid target_thread;
}__attribute__((packed)) g_ui_component_set_listener_request;

typedef struct {
	g_ui_message_header header;
	g_ui_protocol_status status;
}__attribute__((packed)) g_ui_component_set_listener_response;

/**
 * Event structures
 */
typedef struct {
	g_ui_message_header header;
	g_ui_component_event_type type;
	g_ui_component_id component_id;
}__attribute__((packed)) g_ui_component_event_header;

typedef struct {
	g_ui_component_event_header header;
}__attribute__((packed)) g_ui_component_action_event;

typedef struct {
	g_ui_component_event_header header;
	g_rectangle bounds;
}__attribute__((packed)) g_ui_component_bounds_event;

#endif
