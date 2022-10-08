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

#ifndef __WINDOWSERVER_COMPONENTS_DESKTOPITEM__
#define __WINDOWSERVER_COMPONENTS_DESKTOPITEM__

#include "components/component.hpp"
#include "components/label.hpp"
#include <cairo/cairo.h>
#include <libwindow/metrics/point.hpp>
#include <libwindow/metrics/rectangle.hpp>

class background_t;

class desktop_item_t : public component_t
{
  private:
	background_t* background;

	label_t* label;
	bool hovered = false;
	cairo_surface_t* surface = 0;

	g_point pressLocation;
	g_point pressOffset;

	std::string title;
	std::string icon;

  public:
	desktop_item_t(background_t* background, std::string title, std::string program, std::string icon);

	std::string program;

	virtual ~desktop_item_t()
	{
	}

	virtual component_t* handleMouseEvent(mouse_event_t& e);
	virtual void layout();
	virtual void paint();
};

#endif
