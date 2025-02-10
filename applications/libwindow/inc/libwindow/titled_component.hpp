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

#ifndef __LIBWINDOW_TITLED_COMPONENT__
#define __LIBWINDOW_TITLED_COMPONENT__

#include <bits/std_function.h>

#include "interface.hpp"
#include "component.hpp"

class g_titled_component : virtual public g_component
{
protected:
    explicit g_titled_component(g_ui_component_id id) : g_component(id)
    {
    }

public:
    ~g_titled_component() override = default;

    virtual bool setTitle(std::string title);
    virtual std::string getTitle();

    virtual void addTitleListener(std::function<void(std::string)> callback);
};

#endif
