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

#include <components/checkbox.hpp>
#include <events/mouse_event.hpp>

/**
 *
 */
checkbox_t::checkbox_t() :
		component_t(true), checked(false), boxSize(DEFAULT_BOX_SIZE), boxTextGap(DEFAULT_BOX_TEXT_GAP), hovered(false), pressed(false) {
	addChild(&label, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
}

/**
 *
 */
void checkbox_t::layout() {
	g_dimension preferredSize = label.getPreferredSize();
	if (preferredSize.height < boxSize + boxTextGap) {
		preferredSize.height = boxSize + boxTextGap;
	}
	preferredSize.width += preferredSize.height;
	setPreferredSize(preferredSize);
}

/**
 *
 */
void checkbox_t::paint() {

	// TODO
	/*
	 g_rectangle bounds = getBounds();
	 g_painter p(graphics);
	 p.setColor(pressed ? RGB(240, 240, 240) : (hovered ? RGB(245, 245, 255) : RGB(255, 255, 255)));
	 p.fill(g_rectangle(0, 0, boxSize, boxSize));
	 p.setColor((hovered || pressed) ? RGB(140, 140, 150) : RGB(160, 160, 170));
	 p.draw(g_rectangle(0, 0, boxSize - 1, boxSize - 1));

	 if (checked) {
	 p.setColor(RGB(70, 180, 255));
	 g_polygon polygon;
	 polygon.addPoint(5, boxSize / 2 - 3);
	 polygon.addPoint(2, boxSize / 2);
	 polygon.addPoint(boxSize / 2 - 1, boxSize - 4);
	 polygon.addPoint(boxSize, 4);
	 polygon.addPoint(boxSize - 3, 1);
	 polygon.addPoint(boxSize / 2 - 1, boxSize / 2);
	 polygon.translate(-1, 2);
	 p.fill(polygon);
	 }
	 */
}

/**
 *
 */
bool checkbox_t::handle(event_t& e) {

	mouse_event_t* me = dynamic_cast<mouse_event_t*>(&e);
	if (me) {
		if (me->type == G_MOUSE_EVENT_ENTER) {
			hovered = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == G_MOUSE_EVENT_LEAVE) {
			hovered = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == G_MOUSE_EVENT_PRESS) {
			pressed = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == G_MOUSE_EVENT_RELEASE || me->type == G_MOUSE_EVENT_DRAG_RELEASE) {
			pressed = false;

			g_rectangle minbounds = getBounds();
			minbounds.x = 0;
			minbounds.y = 0;
			if (me->type == G_MOUSE_EVENT_RELEASE && minbounds.contains(me->position)) {
				checked = !checked;
			}

			markFor(COMPONENT_REQUIREMENT_PAINT);
		}
		return true;
	}

	return false;
}

/**
 *
 */
void checkbox_t::handleBoundChange(g_rectangle oldBounds) {
	g_rectangle unpositioned = getBounds();
	unpositioned.x = boxSize + boxTextGap;
	unpositioned.y = 0;
	this->label.setBounds(unpositioned);
}
