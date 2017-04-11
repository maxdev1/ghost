/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __COMPONENT__
#define __COMPONENT__

#include "desktop.hpp"
#include <cairo/cairo.h>

#include <ghostuser/graphics/metrics/rectangle.hpp>
#include <ghostuser/graphics/text/text_layouter.hpp>
#include <ghostuser/graphics/text/font_loader.hpp>

/**
 *
 */
class component_t {
public:
	g_rectangle bounds;
	desktop_t* desktop;

	component_t(desktop_t* desktop) :
			desktop(desktop) {
	}
	virtual ~component_t() {
	}

	virtual void paint(cairo_t* cr) = 0;
	virtual void handle_mouse_event(g_ui_component_mouse_event* e) = 0;
};

#endif
