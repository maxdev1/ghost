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

#ifndef __LIBWINDOW_BOUNDING_COMPONENT__
#define __LIBWINDOW_BOUNDING_COMPONENT__

#include "listener/bounds_listener.hpp"

/**
 * Component that is capable of receiving bounds events
 */
class g_bounding_component
{
protected:
    g_component* self;

public:
    explicit g_bounding_component(g_component* self) : self(self)
    {
    }

    virtual ~g_bounding_component() = default;

    bool setBoundsListener(g_bounds_listener* l);
    bool setBoundsListener(g_bounds_listener_func func);
};

#endif
