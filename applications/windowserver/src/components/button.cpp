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

#include <components/button.hpp>
#include <components/window.hpp>

#include <events/focus_event.hpp>
#include <events/mouse_event.hpp>
#include <ghostuser/graphics/text/text_alignment.hpp>
#include <ghostuser/ui/properties.hpp>
#include <ghostuser/ui/interface_specification.hpp>
#include <ghostuser/utils/logger.hpp>
#include <math.h>

/**
 *
 */
button_t::button_t() :
		insets(g_insets(0, 0, 0, 0)), action_component_t(this) {
	enabled = true;
	addChild(&label, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	label.setAlignment(g_text_alignment::CENTER);
}

/**
 *
 */
void button_t::layout() {
	g_dimension labelPreferred = label.getPreferredSize();

	labelPreferred.width += insets.left + insets.right;
	labelPreferred.height += insets.top + insets.bottom;
	setPreferredSize(labelPreferred);
}

/**
 *
 */
void button_t::paint() {

	auto cr = graphics.getContext();
	clearSurface();
	auto bounds = getBounds();

	// choose colors
	g_color_argb background;
	if (enabled) {
		background = state.pressed ? RGB(230, 230, 230) : (state.hovered ? RGB(255, 255, 255) : RGB(248, 248, 248));
	} else {
		background = RGB(200, 200, 200);
	}

	g_color_argb border;
	if (enabled) {
		if (state.focused) {
			border = RGB(55, 155, 255);
		} else {
			border = RGB(180, 180, 180);
		}
	} else {
		border = RGB(160, 160, 160);
	}

	// prepare
	double x = 0.5;
	double y = 0.5;
	double width = bounds.width - 1;
	double height = bounds.height - 1;
	double radius = 2.5;
	double degrees = M_PI / 180.0;

	cairo_new_sub_path(cr);
	cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);

	cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(background));
	cairo_fill_preserve(cr);
	cairo_set_source_rgba(cr, G_COLOR_ARGB_TO_FPARAMS(border));
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
}

/**
 *
 */
bool button_t::handle(event_t& e) {

	mouse_event_t* me = dynamic_cast<mouse_event_t*>(&e);
	if (me) {
		if (enabled) {
			if (me->type == G_MOUSE_EVENT_ENTER) {
				state.hovered = true;
				markFor(COMPONENT_REQUIREMENT_PAINT);

			} else if (me->type == G_MOUSE_EVENT_LEAVE) {
				state.hovered = false;
				markFor(COMPONENT_REQUIREMENT_PAINT);

			} else if (me->type == G_MOUSE_EVENT_PRESS) {
				state.pressed = true;
				markFor(COMPONENT_REQUIREMENT_PAINT);

			} else if (me->type == G_MOUSE_EVENT_RELEASE || me->type == G_MOUSE_EVENT_DRAG_RELEASE) {
				state.pressed = false;
				markFor(COMPONENT_REQUIREMENT_PAINT);

				if (me->type == G_MOUSE_EVENT_RELEASE) {
					if (me->position.x >= 0 && me->position.y >= 0 && me->position.x < getBounds().width && me->position.y < getBounds().height) {
						fireAction();
					}
				}
			}
		}
		return true;
	}

	focus_event_t* fe = dynamic_cast<focus_event_t*>(&e);
	if (fe) {
		if (enabled) {
			if (fe->type == FOCUS_EVENT_GAINED) {
				state.focused = true;
				markFor(COMPONENT_REQUIREMENT_PAINT);
				return true;
			} else if (fe->type == FOCUS_EVENT_LOST) {
				state.focused = false;
				markFor(COMPONENT_REQUIREMENT_PAINT);
				return true;
			}
		}
	}

	return false;
}

/**
 *
 */
void button_t::handleBoundChange(g_rectangle oldBounds) {
	g_rectangle labelBounds = getBounds();
	labelBounds.x = insets.left;
	labelBounds.y = insets.right;
	this->label.setBounds(labelBounds);
}

/**
 *
 */
void button_t::setTitle(std::string title) {
	this->label.setTitle(title);
}

/**
 *
 */
std::string button_t::getTitle() {
	return this->label.getTitle();
}

/**
 *
 */
bool button_t::getNumericProperty(int property, uint32_t* out) {

	if (property == G_UI_PROPERTY_ENABLED) {
		*out = enabled;
		return true;
	}

	return false;
}

/**
 *
 */
bool button_t::setNumericProperty(int property, uint32_t value) {

	if (property == G_UI_PROPERTY_ENABLED) {
		enabled = value;
		state.focused = false;
		markFor(COMPONENT_REQUIREMENT_ALL);
		return true;
	}

	return component_t::setNumericProperty(property,value);
}
