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

#include "components/desktop/item.hpp"
#include "components/desktop/item_container.hpp"
#include <cairo/cairo.h>
#include <math.h>

static void roundedRectangle(cairo_t* cr, double x, double y, double width, double height, double radius)
{
	double degrees = M_PI / 180.0;
	cairo_new_sub_path(cr);
	cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);
}

item_t::item_t(item_container_t* container, std::string title, std::string program, std::string icon)
{
	this->container = container;
	this->title = title;
	this->program = program;
	this->icon = icon;

	label = new label_t();
	component_t::addChild(label);

	label->setTitle(title);
	label->setColor(ARGB(255, 255, 255, 255));
	label->setBounds(g_rectangle(0, 0, 100, 100));
	surface = cairo_image_surface_create_from_png(this->icon.c_str());
}

void item_t::layout()
{
	g_rectangle bounds = getBounds();
	int padding = 10;
	int labelHeight = 20;
	label->setBounds(g_rectangle(padding, bounds.height - labelHeight - 5, bounds.width - padding * 2, labelHeight));
	label->setAlignment(g_text_alignment::CENTER);
}

void item_t::paint()
{
	cairo_t* cr = graphics.getContext();
	if(!cr)
		return;

	auto bounds = getBounds();

	cairo_save(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);

	if(hovered || selected)
	{
		roundedRectangle(cr, 3, 3, bounds.width - 6, bounds.height - 6, 5);
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, selected ? 0.5 : 0.3);
		cairo_fill(cr);
	}

	if(surface)
	{
		int iconWidth = cairo_image_surface_get_width(surface);
		int iconX = bounds.width / 2 - iconWidth / 2;
		int iconY = 10;
		cairo_set_source_surface(cr, surface, iconX, iconY);
		cairo_rectangle(cr, iconX, iconY, bounds.width, bounds.height);
		cairo_fill(cr);
	}
}

void spawnerThread(item_t* item)
{
	g_spawn(item->program.c_str(), "", "", G_SECURITY_LEVEL_APPLICATION);
}

void item_t::setSelected(bool selected)
{
	this->selected = selected;
	markFor(COMPONENT_REQUIREMENT_PAINT);
}

component_t* item_t::handleMouseEvent(mouse_event_t& e)
{
	if(e.type == G_MOUSE_EVENT_ENTER)
	{
		hovered = true;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(e.type == G_MOUSE_EVENT_LEAVE)
	{
		hovered = false;
		markFor(COMPONENT_REQUIREMENT_PAINT);
	}
	else if(e.type == G_MOUSE_EVENT_PRESS)
	{
		if(e.clickCount == 2)
		{
			// TODO This doesn't belong here
			g_create_thread_d((void*) &spawnerThread, this);
		}
		else
		{
			if(!this->selected)
				container->unselectItems();
			container->pressDesktopItems(e.screenPosition);
			this->setSelected(true);
		}
	}
	else if(e.type == G_MOUSE_EVENT_DRAG)
	{
		container->dragDesktopItems(e.screenPosition);
	}
	else if(e.type == G_MOUSE_EVENT_DRAG_RELEASE)
	{
		container->tidyDesktopItems();
	}

	return this;
}

void item_t::onContainerItemPressed(const g_point& screenPosition)
{
	pressOffset = screenPosition - this->getBounds().getStart();
	pressLocation = this->getBounds().getStart();
}

void item_t::onContainerItemDragged(const g_point& screenPosition)
{
	auto bounds = this->getBounds();
	bounds.x = screenPosition.x - pressOffset.x;
	bounds.y = screenPosition.y - pressOffset.y;
	this->setBounds(bounds);
}

void item_t::tidyPosition()
{
	auto bounds = this->getBounds();
	auto bgBounds = container->getBounds();

	int gridScale = container->getGridScale();
	bounds.x += gridScale / 2;
	bounds.x = bounds.x - bounds.x % gridScale;
	if(bounds.x < 0)
		bounds.x = 0;
	else if(bounds.x > bgBounds.width - bounds.width)
		bounds.x = bgBounds.width - bounds.width;

	bounds.y += gridScale / 2;
	bounds.y = bounds.y - bounds.y % gridScale;
	if(bounds.y < 0)
		bounds.y = 0;
	else if(bounds.y > bgBounds.height - bounds.height)
		bounds.y = bgBounds.height - bounds.height;

	this->setBounds(bounds);
}
