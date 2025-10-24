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

#ifndef LIBWINDOW_BUTTON
#define LIBWINDOW_BUTTON

#include "action_component.hpp"
#include "component.hpp"
#include "titled_component.hpp"

typedef uint8_t g_button_style;
#define G_BUTTON_STYLE_DEFAULT  0
#define G_BUTTON_STYLE_GHOST    1

class g_button : virtual public g_component, virtual public g_titled_component, virtual public g_action_component
{
public:
    explicit g_button(g_ui_component_id id) : g_component(id), g_titled_component(id), g_action_component(id)
    {
    }

    static g_button* create();

    void setEnabled(bool enabled);
    bool isEnabled();

    void setStyle(g_button_style style);
    g_button_style getStyle();
};

#endif
