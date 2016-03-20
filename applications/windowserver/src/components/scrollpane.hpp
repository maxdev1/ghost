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

#ifndef __SCROLLPANE__
#define __SCROLLPANE__

#include <components/component.hpp>
#include <components/panel.hpp>

/**
 *
 */
class scrollpane_t: public component_t {
private:
	g_color_argb background;
	panel_t content_panel;
	g_point scroll_point;

public:
	scrollpane_t() :
			background(RGB(240, 240, 240)), scroll_point(g_point(0, 0)) {
		component_t::addChild(&content_panel);
	}

	virtual bool handle(event_t& event);

	virtual void addChild(component_t* comp);

	virtual void handleBoundChange(g_rectangle oldBounds);
};

#endif
