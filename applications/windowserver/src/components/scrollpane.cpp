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

#include <components/scrollpane.hpp>
#include <ghostuser/graphics/painter.hpp>
#include <events/mouse_event.hpp>
#include <stdio.h>

/**
 *
 */
bool scrollpane_t::handle(event_t& event) {

	mouse_event_t* me = dynamic_cast<mouse_event_t*>(&event);
	if (me) {
		if (me->type == MOUSE_EVENT_DRAG) {
			scroll_point.x = -me->position.x;
			scroll_point.y = -me->position.y;

			g_dimension preferred = content_panel.getPreferredSize();
			content_panel.setBounds(g_rectangle(scroll_point.x, scroll_point.y, preferred.width, preferred.height));
		}
		return true;
	}

	return false;
}

/**
 *
 */
void scrollpane_t::handleBoundChange(g_rectangle oldBounds) {
	content_panel.setBounds(getBounds());
}

/**
 *
 */
void scrollpane_t::addChild(component_t* comp) {
	content_panel.addChild(comp);
}
