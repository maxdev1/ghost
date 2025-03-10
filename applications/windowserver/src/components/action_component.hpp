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

#ifndef __WINDOWSERVER_COMPONENTS_ACTIONCOMPONENT__
#define __WINDOWSERVER_COMPONENTS_ACTIONCOMPONENT__
#include "component.hpp"

class action_component_t;

/**
 * Used so the window server can itself be the handler for an action component.
 */
class internal_action_handler_t
{
public:
    virtual ~internal_action_handler_t() = default;

    virtual void handle(action_component_t* source) = 0;
};

/**
 * An action component is capable of being observed by an action listener.
 * The component may fire actions which are dispatched to the registered
 * listener for processing.
 */
class action_component_t : virtual public component_t
{
    internal_action_handler_t* internalHandler;

public:
    explicit action_component_t() : internalHandler(nullptr)
    {
    }

    ~action_component_t() override = default;

    virtual void fireAction();
    virtual void setInternalActionHandler(internal_action_handler_t* handler);
};

#endif
