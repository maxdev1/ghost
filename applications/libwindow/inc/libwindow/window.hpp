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

#ifndef LIBWINDOW_WINDOW
#define LIBWINDOW_WINDOW

#include "component.hpp"
#include "focusable_component.hpp"
#include "titled_component.hpp"

class g_window :
    virtual public g_component,
    virtual public g_titled_component,
    virtual public g_focusable_component
{
public:
    explicit g_window(g_ui_component_id id):
        g_component(id), g_titled_component(id), g_focusable_component(id)
    {
    }

    static g_window* create();
    static g_window* attach(g_ui_component_id id);

    bool isResizable();
    void setResizable(bool resizable);

    bool onClose(std::function<void()> func);
};

#endif
