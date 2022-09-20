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

#ifndef __WINDOWSERVER_EVENTS_FOCUSEVENT__
#define __WINDOWSERVER_EVENTS_FOCUSEVENT__

#include "events/event.hpp"
#include <stdint.h>

enum focus_event_type_t
{
    FOCUS_EVENT_NONE,
    FOCUS_EVENT_GAINED,
    FOCUS_EVENT_LOST
};

class focus_event_t : public event_t
{
  public:
    focus_event_type_t type;
    component_t* newFocusedComponent;

    focus_event_t() : type(FOCUS_EVENT_NONE), newFocusedComponent(nullptr)
    {
    }

    virtual ~focus_event_t() {}

    virtual bool visit(component_t* component);
};

#endif
