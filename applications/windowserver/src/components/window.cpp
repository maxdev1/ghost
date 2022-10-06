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

#include "components/window.hpp"
#include "components/cursor.hpp"
#include "events/focus_event.hpp"
#include "events/key_event.hpp"
#include "events/mouse_event.hpp"
#include "windowserver.hpp"

#include <cairo/cairo.h>
#include <libproperties/parser.hpp>
#include <libwindow/properties.hpp>
#include <libwindow/text/font_manager.hpp>
#include <math.h>

window_t::window_t() : backgroundColor(RGB(240, 240, 240)), borderWidth(DEFAULT_BORDER_WIDTH), cornerSize(DEFAULT_CORNER_SIZE)
{
	visible = false;
	resizable = true;
	crossHovered = false;
	crossPressed = false;
	focused = false;

	shadowSize = 5;
	padding = 5;

	graphics.setAverageFactor(250);

	component_t::addChild(&label, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	component_t::addChild(&panel, COMPONENT_CHILD_REFERENCE_TYPE_INTERNAL);
	setMinimumSize(g_dimension(100, 40));

	panel.setBackground(ARGB(0, 0, 0, 0));
}

void window_t::layout()
{
	g_rectangle bounds = getBounds();
	int titleHeight = 30;
	label.setBounds(g_rectangle(padding + 10, padding, bounds.width - padding - 20, titleHeight));
	panel.setBounds(g_rectangle(padding, padding + titleHeight, bounds.width - padding * 2, bounds.height - titleHeight - padding * 2));
	crossBounds = g_rectangle(bounds.width - 35, 17, 15, 15);
}

/**
 * TODO: Remove subpanel method and let the client do the work here
 */
void window_t::addChild(component_t* component, component_child_reference_type_t type)
{
	panel.addChild(component);
}
void window_t::setLayoutManager(layout_manager_t* layoutManager)
{
	panel.setLayoutManager(layoutManager);
}

void roundedRectangle(cairo_t* cr, double x, double y, double width, double height, double radius)
{
	double degrees = M_PI / 180.0;
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);
}

void window_t::paint()
{
	g_rectangle bounds = getBounds();
	cairo_t* cr = graphics.getContext();
	clearSurface();

	// draw shadow
	double shadowAlpha = 0.003;
	double shadowAlphaAdd = 0.003;
	double addIncr;
	if(focused)
	{
		addIncr = 0.003;
	}
	else
	{
		addIncr = 0.001;
	}
	for(int i = 0; i < shadowSize + 4; i++)
	{
		roundedRectangle(cr, i, i + 3, bounds.width - 2 * i, bounds.height - 2 * i, shadowSize);
		cairo_set_source_rgba(cr, 0, 0, 0, shadowAlpha);
		cairo_stroke(cr);
		shadowAlpha += shadowAlphaAdd;
		shadowAlphaAdd += addIncr;
	}

	// draw background
	roundedRectangle(cr, shadowSize, shadowSize, bounds.width - 2 * shadowSize, bounds.height - 2 * shadowSize, shadowSize);
	cairo_set_source_rgba(cr,
						  ARGB_FR_FROM(backgroundColor),
						  ARGB_FG_FROM(backgroundColor),
						  ARGB_FB_FROM(backgroundColor),
						  ARGB_FA_FROM(backgroundColor) * (focused ? 1.0 : 0.95));
	cairo_fill(cr);

	// draw cross
	if(crossPressed && crossHovered)
	{
		cairo_set_source_rgba(cr, 0.0, 0.4, 0.8, 1);
	}
	else if(crossHovered)
	{
		cairo_set_source_rgba(cr, 0.1, 0.5, 0.9, 1);
	}
	else
	{
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

component_t* window_t::handleFocusEvent(focus_event_t& fe)
{
	if(fe.newFocusedComponent)
	{
		this->focused = (this == fe.newFocusedComponent) || fe.newFocusedComponent->isChildOf(this);
	}
	else
	{
		this->focused = false;
	}
	markFor(COMPONENT_REQUIREMENT_PAINT);
	return this;
}

component_t* window_t::handleMouseEvent(mouse_event_t& me)
{
	g_rectangle currentBounds = getBounds();

	if(me.type == G_MOUSE_EVENT_DRAG)
	{
		if(crossPressed)
		{
			crossHovered = crossBounds.contains(me.position);
			markFor(COMPONENT_REQUIREMENT_PAINT);
		}
		else
		{
			// Window dragging/resizing
			g_point newLocation = me.screenPosition - pressPoint;

			// Calculate new bounds
			g_rectangle newBounds = currentBounds;

			if(resizeMode == RESIZE_MODE_TOP_LEFT)
			{
				newBounds.x = newLocation.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);
			}
			else if(resizeMode == RESIZE_MODE_TOP_RIGHT)
			{
				newBounds.x = pressBounds.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);
			}
			else if(resizeMode == RESIZE_MODE_BOTTOM_LEFT)
			{
				newBounds.x = newLocation.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);
			}
			else if(resizeMode == RESIZE_MODE_BOTTOM_RIGHT)
			{
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);
			}
			else if(resizeMode == RESIZE_MODE_TOP)
			{
				newBounds.x = pressBounds.x;
				newBounds.y = newLocation.y;
				newBounds.width = pressBounds.width;
				newBounds.height = pressBounds.height + (pressBounds.y - newLocation.y);
			}
			else if(resizeMode == RESIZE_MODE_LEFT)
			{
				newBounds.x = newLocation.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width + (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height;
			}
			else if(resizeMode == RESIZE_MODE_BOTTOM)
			{
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width;
				newBounds.height = pressBounds.height - (pressBounds.y - newLocation.y);
			}
			else if(resizeMode == RESIZE_MODE_RIGHT)
			{
				newBounds.x = pressBounds.x;
				newBounds.y = pressBounds.y;
				newBounds.width = pressBounds.width - (pressBounds.x - newLocation.x);
				newBounds.height = pressBounds.height;
			}
			else if(resizeMode == RESIZE_MODE_MOVE)
			{
				newBounds.x = newLocation.x;
				newBounds.y = newLocation.y;
			}

			// Apply bounds
			g_rectangle appliedBounds = getBounds();
			if(newBounds.width > 50)
			{
				appliedBounds.x = newBounds.x;
				appliedBounds.width = newBounds.width;
			}
			if(newBounds.height > 20)
			{
				appliedBounds.y = newBounds.y;
				appliedBounds.height = newBounds.height;
			}
			this->setBounds(appliedBounds);
		}
		return this;
	}

	// Let child components handle other input first
	component_t* handledByChild = component_t::handleMouseEvent(me);
	if(handledByChild)
		return handledByChild;

	// Handle mouse events
	if(me.type == G_MOUSE_EVENT_MOVE)
	{
		if(resizable)
		{
			g_point pos = me.position;
			if((pos.x < shadowSize + cornerSize / 2) && (pos.x > shadowSize - cornerSize / 2) && (pos.y < cornerSize) && (pos.y > shadowSize - cornerSize / 2))
			{ // Top left corner
				cursor_t::set("resize-nwes");
			}
			else if((pos.x > currentBounds.width - shadowSize - cornerSize / 2) && (pos.x < currentBounds.width - shadowSize + cornerSize / 2) && (pos.y < cornerSize) && (pos.y > shadowSize - cornerSize / 2))
			{ // Top right corner
				cursor_t::set("resize-nesw");
			}
			else if((pos.x < shadowSize + cornerSize / 2) && (pos.x > shadowSize - cornerSize / 2) && (pos.y > currentBounds.height - shadowSize - cornerSize / 2) && (pos.y < currentBounds.height - shadowSize + cornerSize / 2))
			{ // Bottom left corner
				cursor_t::set("resize-nesw");
			}
			else if((pos.x > currentBounds.width - shadowSize - cornerSize / 2) && (pos.x < currentBounds.width - shadowSize + cornerSize / 2) && (pos.y > currentBounds.height - shadowSize - cornerSize / 2) && (pos.y < currentBounds.height - shadowSize + cornerSize / 2))
			{ // Bottom right corner
				cursor_t::set("resize-nwes");
			}
			else if(pos.y < shadowSize + borderWidth / 2 && pos.y > shadowSize - borderWidth / 2)
			{ // Top edge
				cursor_t::set("resize-ns");
			}
			else if(pos.x < shadowSize + borderWidth / 2 && pos.x > shadowSize - borderWidth / 2)
			{ // Left edge
				cursor_t::set("resize-ew");
			}
			else if((pos.y > currentBounds.height - shadowSize - borderWidth / 2) && (pos.y < currentBounds.height - shadowSize + borderWidth / 2))
			{ // Bottom edge
				cursor_t::set("resize-ns");
			}
			else if((pos.x > currentBounds.width - shadowSize - borderWidth / 2) && (pos.x < currentBounds.width - shadowSize + borderWidth / 2))
			{ // Right edge
				cursor_t::set("resize-ew");
			}
			else
			{
				cursor_t::set("default");
			}
		}

		if(crossBounds.contains(me.position))
		{
			crossHovered = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
		}
		else
		{
			if(crossHovered)
			{
				markFor(COMPONENT_REQUIREMENT_PAINT);
			}
			crossHovered = false;
		}
	}
	else if(me.type == G_MOUSE_EVENT_PRESS)
	{

		// Press on the cross
		if(crossBounds.contains(me.position))
		{
			crossPressed = true;
			markFor(COMPONENT_REQUIREMENT_PAINT);
			return this;
		}
		else
		{
			// Window drag and resize
			pressPoint = me.position;
			pressBounds = currentBounds;

			resizeMode = RESIZE_MODE_NONE;

			if(resizable)
			{

				if((pressPoint.x < shadowSize + cornerSize / 2) && (pressPoint.x > shadowSize - cornerSize / 2) && (pressPoint.y < cornerSize) && (pressPoint.y > shadowSize - cornerSize / 2))
				{ // Corner resizing
					resizeMode = RESIZE_MODE_TOP_LEFT;
				}
				else if((pressPoint.x > currentBounds.width - shadowSize - cornerSize / 2) && (pressPoint.x < currentBounds.width - shadowSize + cornerSize / 2) && (pressPoint.y < cornerSize) && (pressPoint.y > shadowSize - cornerSize / 2))
				{
					resizeMode = RESIZE_MODE_TOP_RIGHT;
				}
				else if((pressPoint.x < shadowSize + cornerSize / 2) && (pressPoint.x > shadowSize - cornerSize / 2) && (pressPoint.y > currentBounds.height - shadowSize - cornerSize / 2) && (pressPoint.y < currentBounds.height - shadowSize + cornerSize / 2))
				{
					resizeMode = RESIZE_MODE_BOTTOM_LEFT;
				}
				else if((pressPoint.x > currentBounds.width - shadowSize - cornerSize / 2) && (pressPoint.x < currentBounds.width - shadowSize + cornerSize / 2) && (pressPoint.y > currentBounds.height - shadowSize - cornerSize / 2) && (pressPoint.y < currentBounds.height - shadowSize + cornerSize / 2))
				{
					resizeMode = RESIZE_MODE_BOTTOM_RIGHT;
				}
				else if(pressPoint.y < shadowSize + borderWidth / 2 && pressPoint.y > shadowSize - borderWidth / 2)
				{ // Edge resizing
					resizeMode = RESIZE_MODE_TOP;
				}
				else if(pressPoint.x < shadowSize + borderWidth / 2 && pressPoint.x > shadowSize - borderWidth / 2)
				{
					resizeMode = RESIZE_MODE_LEFT;
				}
				else if((pressPoint.y > currentBounds.height - shadowSize - borderWidth / 2) && (pressPoint.y < currentBounds.height - shadowSize + borderWidth / 2))
				{
					resizeMode = RESIZE_MODE_BOTTOM;
				}
				else if((pressPoint.x > currentBounds.width - shadowSize - borderWidth / 2) && (pressPoint.x < currentBounds.width - shadowSize + borderWidth / 2))
				{
					resizeMode = RESIZE_MODE_RIGHT;
				}
			}

			if(resizeMode == RESIZE_MODE_NONE)
			{
				if(pressPoint.y < 40)
				{
					resizeMode = RESIZE_MODE_MOVE;
				}
			}
		}
	}
	else if(me.type == G_MOUSE_EVENT_DRAG_RELEASE)
	{
		crossPressed = false;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(me.type == G_MOUSE_EVENT_RELEASE)
	{
		if(crossBounds.contains(me.position))
		{
			this->close();
			return this;
		}
	}

	return this;
}

void window_t::close()
{

	event_listener_info_t info;
	if(getListener(G_UI_COMPONENT_EVENT_TYPE_CLOSE, info))
	{
		g_ui_component_close_event posted_event;
		posted_event.header.type = G_UI_COMPONENT_EVENT_TYPE_CLOSE;
		posted_event.header.component_id = info.component_id;
		g_send_message(info.target_thread, &posted_event, sizeof(g_ui_component_close_event));
	}
	setVisible(false);
}

bool window_t::getNumericProperty(int property, uint32_t* out)
{

	if(property == G_UI_PROPERTY_RESIZABLE)
	{
		*out = resizable;
		return true;
	}

	return false;
}

bool window_t::setNumericProperty(int property, uint32_t value)
{

	if(property == G_UI_PROPERTY_RESIZABLE)
	{
		resizable = value;
		return true;
	}

	if(property == G_UI_PROPERTY_BACKGROUND)
	{
		backgroundColor = value;

		uint32_t avg = (ARGB_A_FROM(value) + ARGB_G_FROM(value) + ARGB_B_FROM(value)) / 3;
		label.setColor(avg > 128 ? ARGB(255, 0, 0, 0) : ARGB(255, 255, 255, 255));
		return true;
	}

	return component_t::setNumericProperty(property, value);
}

void window_t::setTitle(std::string title)
{
	label.setTitle(title);
}

std::string window_t::getTitle()
{
	return label.getTitle();
}
