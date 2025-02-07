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

#ifndef __WINDOWSERVER_COMPONENTS_CURSOR__
#define __WINDOWSERVER_COMPONENTS_CURSOR__

#include "components/component.hpp"
#include "events/mouse_event.hpp"

#include <cairo/cairo.h>
#include <cstdio>
#include <fstream>
#include <libwindow/metrics/point.hpp>
#include <map>
#include <sstream>
#include <string.h>

#define FALLBACK_CURSOR_SIZE 10

struct cursor_configuration
{
    std::string name;
    cairo_surface_t* surface;
    g_point hitpoint;
    g_dimension size;
};

class cursor_t
{
  public:
    static g_point position;
    static g_point nextPosition;
    static g_mouse_button pressedButtons;
    static g_mouse_button nextPressedButtons;

    static g_ui_component_id pressedComponent;
    static g_ui_component_id draggedComponent;
    static g_ui_component_id hoveredComponent;
    static g_ui_component_id focusedComponent;

    static void paint(graphics_t* global);

    static g_rectangle getArea();

    static std::string get();
    static void set(std::string name);

    static bool load(std::string cursorPath);
};

#endif
