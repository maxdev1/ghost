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

/**
 * This UI interface specification defines the messages
 * that the active window manager must understand.
 */
#define G_WINDOW_MANAGER_IDENTIFIER					"windowmanager"

/**
 * Declared in the UI unit
 */
extern bool g_ui_ready;

/**
 * Transactions
 */
typedef uint32_t g_ui_transaction_id;

struct g_ui_transaction_data {
	uint8_t waiting;
	uint8_t* data;
	uint32_t length;
};

/**
 * Commands
 */
#define G_UI_COMMAND_OPEN_REQUEST					1
#define G_UI_COMMAND_OPEN_RESPONSE					2

/**
 * A protocol message always starts with the header, the message id
 */
typedef uint8_t g_ui_protocol_command_id;
#define G_UI_PROTOCOL_HEADER_LENGTH									(sizeof(g_ui_protocol_command_id))
const g_ui_protocol_command_id G_UI_PROTOCOL_READY = 1;
const g_ui_protocol_command_id G_UI_PROTOCOL_CREATE_WINDOW = 2;
const g_ui_protocol_command_id G_UI_PROTOCOL_CREATE_COMPONENT = 3;
const g_ui_protocol_command_id G_UI_PROTOCOL_ADD_COMPONENT = 4;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_TITLE = 5;
const g_ui_protocol_command_id G_UI_PROTOCOL_GET_TITLE = 6;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_BOUNDS = 7;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_VISIBLE = 8;
const g_ui_protocol_command_id G_UI_PROTOCOL_SET_ACTION_LISTENER = 9;

typedef uint8_t g_ui_protocol_status;
#define G_UI_PROTOCOL_STATUS_LENGTH									(sizeof(g_ui_protocol_status))
const g_ui_protocol_status G_UI_PROTOCOL_SUCCESS = 0;
const g_ui_protocol_status G_UI_PROTOCOL_FAIL = 1;

/**
 * Lengths of the data parts of each message type
 */
#define G_UI_PROTOCOL_CREATE_WINDOW_REQUEST_LENGTH			0 // 0
#define G_UI_PROTOCOL_CREATE_WINDOW_RESPONSE_LENGTH			5 // 1: status, 4: window-id

#define G_UI_PROTOCOL_CREATE_COMPONENT_REQUEST_LENGTH		4 // 4: component type
#define G_UI_PROTOCOL_CREATE_COMPONENT_RESPONSE_LENGTH		5 // 1: status, 4: component-id

#define G_UI_PROTOCOL_ADD_COMPONENT_REQUEST_LENGTH			8 // 4: parent, 4: child
#define G_UI_PROTOCOL_ADD_COMPONENT_RESPONSE_LENGTH			1 // 1: status

#define G_UI_PROTOCOL_SET_TITLE_REQUEST_LENGTH				8 // 4: component-id, 4: length, ?: title
#define G_UI_PROTOCOL_SET_TITLE_RESPONSE_LENGTH				1 // 1: status

#define G_UI_PROTOCOL_GET_TITLE_REQUEST_LENGTH				4 // 4: component-id
#define G_UI_PROTOCOL_GET_TITLE_RESPONSE_LENGTH				5 // 1: status, 4: length, ?: title

#define G_UI_PROTOCOL_SET_BOUNDS_REQUEST_LENGTH				(4 /*comp-id*/ + 4 /*x*/ + 4 /*y*/ + 4 /*w*/ + 4 /*h*/)
#define G_UI_PROTOCOL_SET_BOUNDS_RESPONSE_LENGTH			1 // 1: status

#define G_UI_PROTOCOL_SET_VISIBLE_REQUEST_LENGTH			(4 /*comp-id*/ + 1 /*visible*/)
#define G_UI_PROTOCOL_SET_VISIBLE_RESPONSE_LENGTH			1 // 1: status

#define G_UI_PROTOCOL_SET_ACTION_LISTENER_REQUEST_LENGTH	(4 /*comp-id*/)
#define G_UI_PROTOCOL_SET_ACTION_LISTENER_RESPONSE_LENGTH	(1 /*status*/ + 4 /*listener-id*/)

/**
 * Component types
 */
#define G_UI_COMPONENT_BUTTON								0
#define G_UI_COMPONENT_LABEL								1
#define G_UI_COMPONENT_TEXTFIELD							2

#endif
