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
#include <events/mouse_event.hpp>
#include <stdio.h>

/**
 *
 */
void scrollpane_t::setViewPort(component_t* component) {
	viewPort = component;
	addChild(component);

	addChild(&horizontalScrollbar, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	horizontalScrollbar.setScrollHandler(this);

	addChild(&verticalScrollbar, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	verticalScrollbar.setScrollHandler(this);
}

/**
 *
 */
void scrollpane_t::layout() {
	auto bounds = getBounds();

	verticalScrollbar.setModelArea(bounds.height, viewPort->getBounds().height);
	verticalScrollbar.setBounds(g_rectangle(bounds.width - 15, 0, 15, bounds.height - 15));

	horizontalScrollbar.setModelArea(bounds.width, viewPort->getBounds().width);
	horizontalScrollbar.setBounds(g_rectangle(0, bounds.height - 15, bounds.width - 15, 15));
}

/**
 *
 */
void scrollpane_t::setPosition(g_point& newPosition) {

	if (viewPort != nullptr) {
		scrollPosition = newPosition;

		g_rectangle viewPortSize = viewPort->getBounds();
		g_rectangle bounds = getBounds();

		// limit if too small
		if (scrollPosition.x > 0) {
			scrollPosition.x = 0;

		} else if (viewPortSize.width < bounds.width) {
			scrollPosition.x = 0;

		} else if (scrollPosition.x + viewPortSize.width < bounds.width) {
			scrollPosition.x = bounds.width - viewPortSize.width;
		}

		if (scrollPosition.y > 0) {
			scrollPosition.y = 0;

		} else if (viewPortSize.height < bounds.height) {
			scrollPosition.y = 0;

		} else if (scrollPosition.y + viewPortSize.height < bounds.height) {
			scrollPosition.y = bounds.height - viewPortSize.height;
		}

		viewPort->setBounds(g_rectangle(scrollPosition.x, scrollPosition.y, viewPortSize.width, viewPortSize.height));
	}

}

/**
 *
 */
void scrollpane_t::handleScroll(scrollbar_t* bar) {

	if (bar == &verticalScrollbar) {
		g_point pos = scrollPosition;
		pos.y = -verticalScrollbar.getModelPosition();
		setPosition(pos);

	} else if (bar == &horizontalScrollbar) {
		g_point pos = scrollPosition;
		pos.x = -horizontalScrollbar.getModelPosition();
		setPosition(pos);
	}
}
