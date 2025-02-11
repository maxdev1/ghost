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

#ifndef __WINDOWSERVER_COMPONENTS_SCREEN__
#define __WINDOWSERVER_COMPONENTS_SCREEN__

#include "components/component.hpp"

#include <libwindow/metrics/rectangle.hpp>

class screen_t: virtual public component_t
{
    g_rectangle invalid;

    bool pressing = false;
    g_point pressPoint;

public:
    ~screen_t() override = default;

    void addChild(component_t* comp,
                  component_child_reference_type_t type = COMPONENT_CHILD_REFERENCE_TYPE_DEFAULT) override;
    void removeChild(component_t* comp) override;

    void sendWindowEvent(g_ui_component_id observerId, window_t* window, g_tid observerThread, bool present);

    /**
     * Overrides the default invalidation method. On the component, this method
     * just dispatches to the parent, but here we must remember the invalidation.
     */
    virtual void markDirty(g_rectangle rect);

    g_rectangle grabInvalid();
};

#endif
