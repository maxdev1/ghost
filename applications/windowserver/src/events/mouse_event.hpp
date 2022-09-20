/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2022, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __WINDOWSERVER_EVENTS_MOUSEEVENT__
#define __WINDOWSERVER_EVENTS_MOUSEEVENT__

#include "events/event.hpp"
#include "events/locatable.hpp"
#include <libwindow/ui.hpp>

class mouse_event_t : public event_t, public locatable_t
{
  public:
    mouse_event_t() : type(G_MOUSE_EVENT_NONE), buttons(G_MOUSE_BUTTON_NONE), clickCount(1)
    {
    }

    virtual ~mouse_event_t() {}

    g_mouse_event_type type;
    g_mouse_button buttons;
    int clickCount;

    virtual bool visit(component_t* component);
};

#endif
