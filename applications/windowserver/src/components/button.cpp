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
#include "events/mouse_event.hpp"

#include <libproperties/parser.hpp>
#include <libwindow/properties.hpp>
#include <libfont/text_alignment.hpp>
#include <math.h>


enum g_button_render_state
{
	DEFAULT = 0,
	HOVER = 1,
	PRESSED = 2,
	DISABLED = 3
};

enum g_button_state_color_type
{
	BACKGROUND = 0,
	BORDER = 1,
	BORDER_FOCUSED = 2
};

std::map<g_button_style,
         std::map<g_button_render_state,
                  std::map<g_button_state_color_type, g_color_argb>
         >
> BUTTON_STYLES = {
		{G_BUTTON_STYLE_DEFAULT, {
				 {
						 DEFAULT, {
								 {BACKGROUND, RGB(248, 248, 248)},
								 {BORDER, RGB(193, 193, 193)},
								 {BORDER_FOCUSED, RGB(84, 149, 255)}
						 }
				 },
				 {
						 HOVER, {
								 {BACKGROUND, RGB(255, 255, 255)},
								 {BORDER, RGB(193, 193, 193)},
								 {BORDER_FOCUSED, RGB(84, 149, 255)}
						 }
				 },
				 {
						 PRESSED, {
								 {BACKGROUND, RGB(230, 230, 230)},
								 {BORDER, RGB(193, 193, 193)},
								 {BORDER_FOCUSED, RGB(84, 149, 255)}
						 }
				 },
				 {
						 DISABLED, {
								 {BACKGROUND, RGB(249, 249, 249)},
								 {BORDER, RGB(234, 234, 234)},
								 {BORDER_FOCUSED, RGB(234, 234, 234)}
						 }
				 }
		 }},
		{G_BUTTON_STYLE_GHOST, {
				 {
						 DEFAULT, {
								 {BACKGROUND, ARGB(0, 0, 0, 0)},
								 {BORDER, ARGB(0, 0, 0, 0)},
								 {BORDER_FOCUSED, ARGB(0, 0, 0, 0)}
						 }
				 },
				 {
						 HOVER, {
								 {BACKGROUND, RGB(255, 255, 255)},
								 {BORDER, RGB(193, 193, 193)},
								 {BORDER_FOCUSED, RGB(84, 149, 255)}
						 }
				 },
				 {
						 PRESSED, {
								 {BACKGROUND, RGB(230, 230, 230)},
								 {BORDER, RGB(193, 193, 193)},
								 {BORDER_FOCUSED, RGB(84, 149, 255)}
						 }
				 },
				 {
						 DISABLED, {
								 {BACKGROUND, ARGB(0, 0, 0, 0)},
								 {BORDER, ARGB(0, 0, 0, 0)},
								 {BORDER_FOCUSED, ARGB(0, 0, 0, 0)}
						 }
				 }
		 }}
};


button_t::button_t() :
	insets(g_insets(5, 10, 5, 10))
{
	enabled = true;
	component_t::addChild(&label, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	label.setAlignment(g_text_alignment::CENTER);
	label.setColor(RGB(10, 10, 15));
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

	auto styleData = BUTTON_STYLES[style];
	auto stateData = enabled
		                 ? state.pressed
			                   ? styleData[PRESSED]
			                   : (state.hovered
				                      ? styleData[HOVER]
				                      : styleData[DEFAULT])
		                 : styleData[DISABLED];
	g_color_argb background = stateData[BACKGROUND];
	g_color_argb border = stateData[focused ? BORDER_FOCUSED : BORDER];

	double offsetX = 0.5;
	double offsetY = 0.5;
	double borderRadius = 5;

	auto bounds = getBounds();
	double width = bounds.width - 1;
	double height = bounds.height - 1;
	double degrees = M_PI / 180.0;

	cairo_new_sub_path(cr);
	cairo_arc(cr, offsetX + width - borderRadius, offsetY + borderRadius, borderRadius, -90 * degrees, 0 * degrees);
	cairo_arc(cr, offsetX + width - borderRadius, offsetY + height - borderRadius, borderRadius, 0 * degrees,
	          90 * degrees);
	cairo_arc(cr, offsetX + borderRadius, offsetY + height - borderRadius, borderRadius, 90 * degrees, 180 * degrees);
	cairo_arc(cr, offsetX + borderRadius, offsetY + borderRadius, borderRadius, 180 * degrees, 270 * degrees);
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

void button_t::setFocusedInternal(bool focused)
{
	focusable_component_t::setFocusedInternal(focused);
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

bool button_t::isFocusableDefault() const
{
	return enabled;
}

void button_t::setTitleInternal(std::string title)
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
	label.setColor(enabled ? RGB(10, 10, 15) : RGB(80, 80, 80));
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

bool button_t::getNumericProperty(int property, uint32_t* out)
{
	if(property == G_UI_PROPERTY_ENABLED)
	{
		*out = enabled;
		return true;
	}
	else if(property == G_UI_PROPERTY_STYLE)
	{
		*out = style;
		return true;
	}

	return false;
}

bool button_t::setNumericProperty(int property, uint32_t value)
{
	if(property == G_UI_PROPERTY_ENABLED)
	{
		setEnabled(value);
		markFor(COMPONENT_REQUIREMENT_ALL);
		return true;
	}
	else if(property == G_UI_PROPERTY_STYLE)
	{
		style = value;
		markFor(COMPONENT_REQUIREMENT_ALL);
		platformLog("set style to %i", value);
		return true;
	}

	return component_t::setNumericProperty(property, value);
}
