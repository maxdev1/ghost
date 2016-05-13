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

#include <components/window.hpp>
#include <windowserver.hpp>
#include <events/focus_event.hpp>
#include <events/key_event.hpp>
#include <events/mouse_event.hpp>
#include <ghostuser/graphics/text/font_manager.hpp>
#include <ghostuser/io/files/file_utils.hpp>
#include <ghostuser/ui/properties.hpp>
#include <cairo/cairo.h>

/**
 *
 */
window_t::window_t() :
		backgroundColor(RGB(240, 240, 240)), borderWidth(DEFAULT_BORDER_WIDTH), cornerSize(DEFAULT_CORNER_SIZE) {
	this->visible = false;
	this->resizable = true;

	component_t::addChild(&label);
	label.setAlignment(g_text_alignment::CENTER);
	component_t::addChild(&panel);
}

/**
 *
 */
void window_t::layout() {
	g_rectangle bounds = getBounds();
	label.setBounds(g_rectangle(0, 3, bounds.width, 25));
	panel.setBounds(g_rectangle(1, 31, bounds.width - 2, bounds.height - 32));
}

/**
 *
 */
void window_t::addChild(component_t* component) {
	panel.addChild(component);
}

/**
 *
 */
void window_t::paint() {

	cairo_t* cr = graphics.getContext();

	clearSurface();

	// fill background
	cairo_set_source_rgba(cr, 0.9, 0.9, 0.9, 0.9);
	cairo_rectangle(cr, 0, 0, getBounds().width, getBounds().height);
	cairo_fill(cr);

	// TODO
	/*
	 p.setColor(backgroundColor);
	 p.fill(g_rectangle(0, 0, getBounds().width, getBounds().height));

	 p.setColor(RGB(230, 230, 230));
	 p.fill(g_rectangle(0, 0, getBounds().width, 30));

	 p.setColor(RGB(180, 180, 180));
	 p.drawLine(g_point(0, 30), g_point(getBounds().width, 30));

	 p.setColor(RGB(80, 80, 80));
	 p.draw(g_rectangle(0, 0, getBounds().width - 1, getBounds().height - 1));
	 */
}

/**
 *
 */
void window_t::handleBoundChange(g_rectangle oldBounds) {
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

/**
 *
 */
bool window_t::handle(event_t& event) {

	// Catch focus event
	focus_event_t* focusEvent = dynamic_cast<focus_event_t*>(&event);
	if (focusEvent) {
		return true;
	}

	// Let components handle input
	if (component_t::handle(event)) {
		return true;
	}

	// Handle dragging / resizing
	static g_point pressPoint;
	static g_rectangle pressBounds;
	static window_resize_mode_t resizeMode;

	mouse_event_t* mouseEvent = dynamic_cast<mouse_event_t*>(&event);
	if (mouseEvent) {

		if (mouseEvent->type == MOUSE_EVENT_DRAG) {

			g_point newLocation = mouseEvent->screenPosition - pressPoint;

			// Calculate new bounds
			g_rectangle newBounds = getBounds();

			if (resizeMode == RESIZE_MODE_TOP_LEFT) {
				newBounds.x = newLocation.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_TOP_RIGHT) {
				newBounds.x = pressBounds.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_BOTTOM_LEFT) {
				newBounds.x = newLocation.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_BOTTOM_RIGHT) {
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_TOP) {
				newBounds.x = pressBounds.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width;
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_LEFT) {
				newBounds.x = newLocation.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height;

			} else if (resizeMode == RESIZE_MODE_BOTTOM) {
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width;
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);

			} else if (resizeMode == RESIZE_MODE_RIGHT) {
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height;

			} else if (resizeMode == RESIZE_MODE_MOVE) {
				newBounds.x = newLocation.x;
				newBounds.y = newLocation.y;
			}

			// Apply bounds
			g_rectangle appliedBounds = getBounds();
			if (newBounds.width > 50) {
				appliedBounds.x = newBounds.x;
				appliedBounds.width = newBounds.width;
			}
			if (newBounds.height > 20) {
				appliedBounds.y = newBounds.y;
				appliedBounds.height = newBounds.height;
			}
			this->setBounds(appliedBounds);

		} else if (mouseEvent->type == MOUSE_EVENT_PRESS) {

			pressPoint = mouseEvent->position;
			pressBounds = getBounds();

			resizeMode = RESIZE_MODE_NONE;

			if (resizable) {
				if ((pressPoint.x < cornerSize) && (pressPoint.y < cornerSize)) { // Corner resizing
					resizeMode = RESIZE_MODE_TOP_LEFT;
				} else if ((pressPoint.x > getBounds().width - cornerSize) && (pressPoint.y < cornerSize)) {
					resizeMode = RESIZE_MODE_TOP_RIGHT;
				} else if ((pressPoint.x < cornerSize) && (pressPoint.y > getBounds().height - cornerSize)) {
					resizeMode = RESIZE_MODE_BOTTOM_LEFT;
				} else if ((pressPoint.x > getBounds().width - cornerSize) && (pressPoint.y > getBounds().height - cornerSize)) {
					resizeMode = RESIZE_MODE_BOTTOM_RIGHT;

				} else if (pressPoint.y < borderWidth) {	// Edge resizing
					resizeMode = RESIZE_MODE_TOP;
				} else if (pressPoint.x < borderWidth) {
					resizeMode = RESIZE_MODE_LEFT;
				} else if (pressPoint.y > getBounds().height - borderWidth) {
					resizeMode = RESIZE_MODE_BOTTOM;
				} else if (pressPoint.x > getBounds().width - borderWidth) {
					resizeMode = RESIZE_MODE_RIGHT;
				}
			}

			if (resizeMode == RESIZE_MODE_NONE) {
				if (pressPoint.y < 30) {
					resizeMode = RESIZE_MODE_MOVE;
				}
			}

		}

	}

	return true;
}

/**
 *
 */
bool window_t::getBoolProperty(int property, bool* out) {

	if (property == G_UI_PROPERTY_RESIZABLE) {
		*out = resizable;
		return true;
	}

	return false;
}

/**
 *
 */
bool window_t::setBoolProperty(int property, bool value) {

	if (property == G_UI_PROPERTY_RESIZABLE) {
		resizable = value;
		return true;
	}

	return false;
}

/**
 *
 */
void window_t::setTitle(std::string title) {
	label.setTitle(title);
}

/**
 *
 */
std::string window_t::getTitle() {
	return label.getTitle();
}
