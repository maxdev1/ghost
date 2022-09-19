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

#ifndef __LIBWINDOW_WINDOW__
#define __LIBWINDOW_WINDOW__

#include <stdint.h>

#define G_WINDOWSERVER_ID "windowserver"

typedef int g_ui_component_id;

/**
 * Types of events that can be listened to
 */
typedef uint32_t g_ui_component_event_type;
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_ACTION = 0;
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_BOUNDS = 1;
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_CANVAS_WFA = 2; // "wait for acknowledge"-event
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_KEY = 3;
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_FOCUS = 4;
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_MOUSE = 5;
const g_ui_component_event_type G_UI_COMPONENT_EVENT_TYPE_CLOSE = 6;

/**
 * Mouse events
 */
typedef uint8_t g_mouse_button;
#define G_MOUSE_BUTTON_NONE ((g_mouse_button) 0x0)
#define G_MOUSE_BUTTON_1 ((g_mouse_button) 0x1)
#define G_MOUSE_BUTTON_2 ((g_mouse_button) 0x2)
#define G_MOUSE_BUTTON_3 ((g_mouse_button) 0x4)

typedef uint8_t g_mouse_event_type;
#define G_MOUSE_EVENT_NONE ((g_mouse_event_type) 0)
#define G_MOUSE_EVENT_MOVE ((g_mouse_event_type) 1)
#define G_MOUSE_EVENT_PRESS ((g_mouse_event_type) 2)
#define G_MOUSE_EVENT_RELEASE ((g_mouse_event_type) 3)
#define G_MOUSE_EVENT_DRAG_RELEASE ((g_mouse_event_type) 4)
#define G_MOUSE_EVENT_DRAG ((g_mouse_event_type) 5)
#define G_MOUSE_EVENT_ENTER ((g_mouse_event_type) 6)
#define G_MOUSE_EVENT_LEAVE ((g_mouse_event_type) 7)

/**
 * Creates a window.
 */
g_ui_component_id windowCreate();

/**
 * Canvas shared memory header
 */
typedef struct
{
    uint16_t paintable_width;
    uint16_t paintable_height;
    uint16_t blit_x;
    uint16_t blit_y;
    uint16_t blit_width;
    uint16_t blit_height;
    g_bool ready;
} __attribute__((packed)) g_ui_canvas_shared_memory_header;

/**
 * Cairo requires the canvas memory buffer to be aligned. This constant must be used to calculate
 * the address for the canvas buffer in the canvas shared memory.
 */
#define G_UI_CANVAS_SHARED_MEMORY_HEADER_SIZE ((sizeof(g_ui_canvas_shared_memory_header) - sizeof(g_ui_canvas_shared_memory_header) % sizeof(g_address)) + sizeof(g_address))

#endif
