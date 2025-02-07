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

#include "components/button.hpp"
#include "components/window.hpp"
#include "events/focus_event.hpp"
#include "events/mouse_event.hpp"

#include <libproperties/parser.hpp>
#include <libwindow/properties.hpp>
#include <libfont/text_alignment.hpp>
#include <math.h>

button_t::button_t() :
	insets(g_insets(5, 10, 5, 10)), action_component_t(this)
{
	enabled = true;
	addChild(&label, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	label.setAlignment(g_text_alignment::CENTER);
}

/**
 * Layouts the button.
 */
void button_t::update()
{
	g_dimension preferred = label.getPreferredSize();
	preferred.width += insets.left + insets.right;
	preferred.height += insets.top + insets.bottom;

	auto min = getMinimumSize();
	if(preferred.height < min.height)
		preferred.height = min.height;
	if(preferred.width < min.width)
		preferred.width = min.width;

	setPreferredSize(preferred);

	markParentFor(COMPONENT_REQUIREMENT_UPDATE);
}

void button_t::layout()
{
	auto bounds = getBounds();
	bounds.x = 0;
	bounds.y = 0;
	bounds -= insets;
	label.setBounds(bounds);
}

void button_t::paint()
{

	auto cr = graphics.acquireContext();
	if(!cr)
		return;

	clearSurface();
	auto bounds = getBounds();

	// choose colors
	g_color_argb background;
	if(enabled)
	{
		background = state.pressed ? RGB(230, 230, 230) : (state.hovered ? RGB(255, 255, 255) : RGB(248, 248, 248));
	}
	else
	{
		background = RGB(200, 200, 200);
	}

	g_color_argb border;
	if(enabled)
	{
		if(state.focused)
		{
			border = RGB(55, 155, 255);
		}
		else
		{
			border = RGB(180, 180, 180);
		}
	}
	else
	{
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

	graphics.releaseContext();
}

component_t* button_t::handleMouseEvent(mouse_event_t& me)
{
	if(!enabled)
		return nullptr;

	if(me.type == G_MOUSE_EVENT_ENTER)
	{
		state.hovered = true;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(me.type == G_MOUSE_EVENT_LEAVE)
	{
		state.hovered = false;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(me.type == G_MOUSE_EVENT_PRESS)
	{
		state.pressed = true;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(me.type == G_MOUSE_EVENT_RELEASE || me.type == G_MOUSE_EVENT_DRAG_RELEASE)
	{
		state.pressed = false;
		markFor(COMPONENT_REQUIREMENT_PAINT);

		if(me.type == G_MOUSE_EVENT_RELEASE)
		{
			if(me.position.x >= 0 && me.position.y >= 0 && me.position.x < getBounds().width && me.position.y <
			   getBounds().height)
			{
				fireAction();
			}
		}
	}
	return this;
}

component_t* button_t::handleFocusEvent(focus_event_t& fe)
{
	if(enabled)
	{
		if(fe.type == FOCUS_EVENT_GAINED)
		{
			state.focused = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			return this;
		}
		else if(fe.type == FOCUS_EVENT_LOST)
		{
			state.focused = false;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			return this;
		}
	}
	return nullptr;
}

void button_t::setTitle(std::string title)
{
	this->label.setTitle(title);
}

std::string button_t::getTitle()
{
	return this->label.getTitle();
}

void button_t::setEnabled(bool enabled)
{
	this->enabled = enabled;
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

bool button_t::getNumericProperty(int property, uint32_t* out)
{
	if(property == G_UI_PROPERTY_ENABLED)
	{
		*out = enabled;
		return true;
	}

	return false;
}

bool button_t::setNumericProperty(int property, uint32_t value)
{
	if(property == G_UI_PROPERTY_ENABLED)
	{
		enabled = value;
		state.focused = false;
		markFor(COMPONENT_REQUIREMENT_ALL);
		return true;
	}

	return component_t::setNumericProperty(property, value);
}
