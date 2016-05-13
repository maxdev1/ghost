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

#include <components/scrollbar.hpp>
#include <events/mouse_event.hpp>
#include <stdio.h>

/**
 *
 */
void scrollbar_t::paint() {

	g_rectangle bounds = getBounds();
	auto cr = graphics.getContext();
	clearSurface();

	g_rectangle knob = calculateKnob();

	// knob
	cairo_rectangle(cr, knob.x, knob.y, knob.width, knob.height);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
	cairo_fill(cr);
}

/**
 *
 */
bool scrollbar_t::handle(event_t& e) {

	mouse_event_t* me = dynamic_cast<mouse_event_t*>(&e);
	if (me) {
		if (me->type == MOUSE_EVENT_ENTER) {
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == MOUSE_EVENT_PRESS) {

			g_rectangle knob = calculateKnob();
			if (knob.contains(me->position)) {
				if (orientation == scrollbar_orientation_t::VERTICAL) {
					dragPressPosition = me->position.y;
					dragViewPosition = knob.y;
				} else if (orientation == scrollbar_orientation_t::HORIZONTAL) {
					dragPressPosition = me->position.x;
					dragViewPosition = knob.x;
				}
			}
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (me->type == MOUSE_EVENT_DRAG) {

			int mousePosition;
			if (orientation == scrollbar_orientation_t::VERTICAL) {
				mousePosition = me->position.y;
			} else if (orientation == scrollbar_orientation_t::HORIZONTAL) {
				mousePosition = me->position.x;
			}

			int viewPosition = dragViewPosition + (mousePosition - dragPressPosition);
			int viewMax = getViewMax();
			if (viewPosition < 0) {
				viewPosition = 0;
			} else if (viewPosition > viewMax) {
				viewPosition = viewMax;
			}

			if (viewMax == 0) {
				viewMax = 1;
			}
			int modelMax = modelTotalArea - modelVisibleArea;
			modelPosition = (viewPosition * modelMax) / viewMax;
			if (modelPosition < 0) {
				modelPosition = 0;
			} else if (modelPosition > modelMax) {
				modelPosition = modelMax;
			}

			if (scrollHandler) {
				scrollHandler->handleScroll(this);
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
void scrollbar_t::setModelArea(int visible, int total) {
	modelVisibleArea = visible;
	modelTotalArea = total;

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
void scrollbar_t::setModelPosition(int pos) {
	modelPosition = pos;

	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
g_rectangle scrollbar_t::calculateKnob() {

	g_rectangle bounds = getBounds();

	// calculate knob size
	int knobSize = getKnobSize();

	// calculate area that knob can scroll on
	int viewMax = getViewMax();
	int modelMax = modelTotalArea - modelVisibleArea;
	if (modelMax == 0) {
		modelMax = 1;
	}
	int viewPosition = (viewMax * modelPosition) / modelMax;

	// create rectangle for knob
	if (orientation == scrollbar_orientation_t::VERTICAL) {
		return g_rectangle(0, viewPosition, bounds.width, knobSize);

	} else if (orientation == scrollbar_orientation_t::HORIZONTAL) {
		return g_rectangle(viewPosition, 0, knobSize, bounds.height);
	}

	// dummy
	return g_rectangle(0, 0, 1, 1);
}

/**
 *
 */
int scrollbar_t::getViewMax() {

	if (orientation == scrollbar_orientation_t::VERTICAL) {
		return getBounds().height - getKnobSize();

	} else if (orientation == scrollbar_orientation_t::HORIZONTAL) {
		return getBounds().width - getKnobSize();
	}

	return 0;
}

/**
 *
 */
int scrollbar_t::getKnobSize() {

	int bounds;
	if (orientation == scrollbar_orientation_t::VERTICAL) {
		bounds = getBounds().height;

	} else if (orientation == scrollbar_orientation_t::HORIZONTAL) {
		bounds = getBounds().width;
	}

	int size = (bounds * modelVisibleArea) / modelTotalArea;
	if (size < 20) {
		size = 20;
	}
	return size;
}
