/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef LIBWINDOW_CANVASBUFFERLISTENERINTERNAL
#define LIBWINDOW_CANVASBUFFERLISTENERINTERNAL

#include "listener.hpp"

class g_component;

/**
 * Listener that a canvas registers on itself to react when a new buffer is ready.
 */
class g_canvas_buffer_listener_internal : public g_listener
{
public:
    g_canvas* canvas;

    explicit g_canvas_buffer_listener_internal(g_canvas* canvas) : canvas(canvas)
    {
    }

    void process(g_ui_component_event_header* header) override
    {
        if (header->type == G_UI_COMPONENT_EVENT_TYPE_CANVAS_NEW_BUFFER)
        {
            auto event = (g_ui_component_canvas_wfa_event*)header;
            canvas->acknowledgeNewBuffer(event->newBufferAddress, event->width, event->height);
        }
    }
};

#endif
