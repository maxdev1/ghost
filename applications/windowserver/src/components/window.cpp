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
#include <components/cursor.hpp>
#include <math.h>

/**
 *
 */
window_t::window_t() :
		backgroundColor(RGB(240, 240, 240)), borderWidth(DEFAULT_BORDER_WIDTH), cornerSize(DEFAULT_CORNER_SIZE) {
	visible = false;
	resizable = true;
	crossHovered = false;
	crossPressed = false;
	focused = false;

	shadowSize = 10;

	component_t::addChild(&label, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	component_t::addChild(&panel, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	setMinimumSize(g_dimension(100, 40));

	panel.setBackground(ARGB(0, 0, 0, 0));
}

/**
 *
 */
void window_t::layout() {
	g_rectangle bounds = getBounds();
	label.setBounds(g_rectangle(18, 12, bounds.width - 30, 25));
	panel.setBounds(g_rectangle(12, 40, bounds.width - 24, bounds.height - 52));
	crossBounds = g_rectangle(bounds.width - 35, 17, 15, 15);
}

/**
 * TODO: Remove subpanel method and let the client do the work here
 */
void window_t::addChild(component_t* component, component_child_reference_type_t type) {
	panel.addChild(component);
}
void window_t::setLayoutManager(layout_manager_t* layoutManager) {
	panel.setLayoutManager(layoutManager);
}

/**
 *
 */
void roundedRectangle(cairo_t* cr, double x, double y, double width, double height, double radius) {
	double degrees = M_PI / 180.0;
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);
}

/**
 *
 */
void window_t::paint() {

	g_rectangle bounds = getBounds();
	cairo_t* cr = graphics.getContext();
	clearSurface();

	// draw shadow
	double shadowAlpha = 0.003;
	double shadowAlphaAdd = 0.003;
	double addIncr;
	if (focused) {
		addIncr = 0.003;
	} else {
		addIncr = 0.001;
	}
	for (int i = 0; i < shadowSize + 4; i++) {
		roundedRectangle(cr, i, i + 3, bounds.width - 2 * i, bounds.height - 2 * i, 10);
		cairo_set_source_rgba(cr, 0, 0, 0, shadowAlpha);
		cairo_stroke(cr);
		shadowAlpha += shadowAlphaAdd;
		shadowAlphaAdd += addIncr;
	}

	// draw background
	double degrees = M_PI / 180.0;
	cairo_new_sub_path(cr);
	double radius = 5;
	cairo_arc(cr, shadowSize + radius, shadowSize + radius, radius, 180 * degrees, 270 * degrees);
	cairo_arc(cr, bounds.width - radius - shadowSize, shadowSize + radius, radius, -90 * degrees, 0 * degrees);
	cairo_line_to(cr, bounds.width - shadowSize, bounds.height - shadowSize);
	cairo_line_to(cr, shadowSize, bounds.height - shadowSize);
	cairo_close_path(cr);
	cairo_set_source_rgba(cr, 0.95, 0.95, 0.95, focused ? 1 : 0.8);
	cairo_fill(cr);

	// draw cross
	if (crossPressed && crossHovered) {
		cairo_set_source_rgba(cr, 0.0, 0.4, 0.8, 1);
	} else if (crossHovered) {
		cairo_set_source_rgba(cr, 0.1, 0.5, 0.9, 1);
	} else {
		cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 1);
	}

	int crossPadding = 3;
	cairo_set_line_width(cr, 1.5);
	cairo_move_to(cr, crossBounds.x + crossPadding, crossBounds.y + crossPadding);
	cairo_line_to(cr, crossBounds.x + crossBounds.width - crossPadding, crossBounds.y + crossBounds.height - crossPadding);
	cairo_stroke(cr);
	cairo_move_to(cr, crossBounds.x + crossBounds.width - crossPadding, crossBounds.y + crossPadding);
	cairo_line_to(cr, crossBounds.x + crossPadding, crossBounds.y + crossBounds.height - crossPadding);
	cairo_stroke(cr);
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
		if (focusEvent->newFocusedComponent) {
			this->focused = (this == focusEvent->newFocusedComponent) || focusEvent->newFocusedComponent->isChildOf(this);
		} else {
			this->focused = false;
		}
		markFor(COMPONENT_REQUIREMENT_PAINT);
		return true;
	}

	// Let components handle input
	if (component_t::handle(event)) {
		return true;
	}

	// Handle mouse events
	static g_point pressPoint;
	static g_rectangle pressBounds;
	static window_resize_mode_t resizeMode;

	mouse_event_t* mouseEvent = dynamic_cast<mouse_event_t*>(&event);
	if (mouseEvent) {
		g_rectangle currentBounds = getBounds();

		if (mouseEvent->type == G_MOUSE_EVENT_MOVE) {
			if (resizable) {
				g_point pos = mouseEvent->position;
				if ((pos.x < shadowSize + cornerSize / 2) && (pos.x > shadowSize - cornerSize / 2) && (pos.y < cornerSize)
						&& (pos.y > shadowSize - cornerSize / 2)) { // Top left corner
					cursor_t::set("resize-nwes");

				} else if ((pos.x > currentBounds.width - shadowSize - cornerSize / 2) && (pos.x < currentBounds.width - shadowSize + cornerSize / 2)
						&& (pos.y < cornerSize) && (pos.y > shadowSize - cornerSize / 2)) { // Top right corner
					cursor_t::set("resize-nesw");

				} else if ((pos.x < shadowSize + cornerSize / 2) && (pos.x > shadowSize - cornerSize / 2)
						&& (pos.y > currentBounds.height - shadowSize - cornerSize / 2) && (pos.y < currentBounds.height - shadowSize + cornerSize / 2)) { // Bottom left corner
					cursor_t::set("resize-nesw");

				} else if ((pos.x > currentBounds.width - shadowSize - cornerSize / 2) && (pos.x < currentBounds.width - shadowSize + cornerSize / 2)
						&& (pos.y > currentBounds.height - shadowSize - cornerSize / 2) && (pos.y < currentBounds.height - shadowSize + cornerSize / 2)) { // Bottom right corner
					cursor_t::set("resize-nwes");

				} else if (pos.y < shadowSize + borderWidth / 2 && pos.y > shadowSize - borderWidth / 2) {	// Top edge
					cursor_t::set("resize-ns");
				} else if (pos.x < shadowSize + borderWidth / 2 && pos.x > shadowSize - borderWidth / 2) { // Left edge
					cursor_t::set("resize-ew");
				} else if ((pos.y > currentBounds.height - shadowSize - borderWidth / 2) && (pos.y < currentBounds.height - shadowSize + borderWidth / 2)) { // Bottom edge
					cursor_t::set("resize-ns");
				} else if ((pos.x > currentBounds.width - shadowSize - borderWidth / 2) && (pos.x < currentBounds.width - shadowSize + borderWidth / 2)) { // Right edge
					cursor_t::set("resize-ew");
				} else {
					cursor_t::set("default");
				}
			}

			// Cross
			if (crossBounds.contains(mouseEvent->position)) {
				crossHovered = true;
			} else {
				crossHovered = false;
			}
			markFor(COMPONENT_REQUIREMENT_PAINT);

		} else if (mouseEvent->type == G_MOUSE_EVENT_DRAG) {
			// Press on the cross
			if (crossPressed) {
				crossHovered = crossBounds.contains(mouseEvent->position);
				markFor(COMPONENT_REQUIREMENT_PAINT);

			} else {

				// Window dragging/resizing
				g_point newLocation = mouseEvent->screenPosition - pressPoint;

				// Calculate new bounds
				g_rectangle newBounds = currentBounds;

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
			}

		} else if (mouseEvent->type == G_MOUSE_EVENT_PRESS) {

			// Press on the cross
			if (crossBounds.contains(mouseEvent->position)) {
				crossPressed = true;
				markFor(COMPONENT_REQUIREMENT_PAINT);

			} else {

				// Window drag and resize
				pressPoint = mouseEvent->position;
				pressBounds = currentBounds;

				resizeMode = RESIZE_MODE_NONE;

				if (resizable) {

					if ((pressPoint.x < shadowSize + cornerSize / 2) && (pressPoint.x > shadowSize - cornerSize / 2) && (pressPoint.y < cornerSize)
							&& (pressPoint.y > shadowSize - cornerSize / 2)) { // Corner resizing
						resizeMode = RESIZE_MODE_TOP_LEFT;
					} else if ((pressPoint.x > currentBounds.width - shadowSize - cornerSize / 2)
							&& (pressPoint.x < currentBounds.width - shadowSize + cornerSize / 2) && (pressPoint.y < cornerSize)
							&& (pressPoint.y > shadowSize - cornerSize / 2)) {
						resizeMode = RESIZE_MODE_TOP_RIGHT;
					} else if ((pressPoint.x < shadowSize + cornerSize / 2) && (pressPoint.x > shadowSize - cornerSize / 2)
							&& (pressPoint.y > currentBounds.height - shadowSize - cornerSize / 2)
							&& (pressPoint.y < currentBounds.height - shadowSize + cornerSize / 2)) {
						resizeMode = RESIZE_MODE_BOTTOM_LEFT;
					} else if ((pressPoint.x > currentBounds.width - shadowSize - cornerSize / 2)
							&& (pressPoint.x < currentBounds.width - shadowSize + cornerSize / 2)
							&& (pressPoint.y > currentBounds.height - shadowSize - cornerSize / 2)
							&& (pressPoint.y < currentBounds.height - shadowSize + cornerSize / 2)) {
						resizeMode = RESIZE_MODE_BOTTOM_RIGHT;

					} else if (pressPoint.y < shadowSize + borderWidth / 2 && pressPoint.y > shadowSize - borderWidth / 2) {	// Edge resizing
						resizeMode = RESIZE_MODE_TOP;
					} else if (pressPoint.x < shadowSize + borderWidth / 2 && pressPoint.x > shadowSize - borderWidth / 2) {
						resizeMode = RESIZE_MODE_LEFT;
					} else if ((pressPoint.y > currentBounds.height - shadowSize - borderWidth / 2)
							&& (pressPoint.y < currentBounds.height - shadowSize + borderWidth / 2)) {
						resizeMode = RESIZE_MODE_BOTTOM;
					} else if ((pressPoint.x > currentBounds.width - shadowSize - borderWidth / 2)
							&& (pressPoint.x < currentBounds.width - shadowSize + borderWidth / 2)) {
						resizeMode = RESIZE_MODE_RIGHT;
					}
				}

				if (resizeMode == RESIZE_MODE_NONE) {
					if (pressPoint.y < 40) {
						resizeMode = RESIZE_MODE_MOVE;
					}
				}
			}

		} else if (mouseEvent->type == G_MOUSE_EVENT_LEAVE) {
			cursor_t::set("default");

		} else if (mouseEvent->type == G_MOUSE_EVENT_DRAG_RELEASE) {
			crossPressed = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);

			if (crossBounds.contains(mouseEvent->position)) {
				this->close();
			}

		}

	}

	return true;
}

/**
 *
 */
void window_t::close() {

	event_listener_info_t info;
	if (getListener(G_UI_COMPONENT_EVENT_TYPE_CLOSE, info)) {
		g_ui_component_close_event posted_event;
		posted_event.header.type = G_UI_COMPONENT_EVENT_TYPE_CLOSE;
		posted_event.header.component_id = info.component_id;
		g_send_message(info.target_thread, &posted_event, sizeof(g_ui_component_close_event));
	}
	setVisible(false);
}

/**
 *
 */
bool window_t::getNumericProperty(int property, uint32_t* out) {

	if (property == G_UI_PROPERTY_RESIZABLE) {
		*out = resizable;
		return true;
	}

	return false;
}

/**
 *
 */
bool window_t::setNumericProperty(int property, uint32_t value) {

	if (property == G_UI_PROPERTY_RESIZABLE) {
		resizable = value;
		return true;
	}

	return component_t::setNumericProperty(property, value);
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

