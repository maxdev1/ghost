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

#include "components/desktop/desktop_item.hpp"
#include "components/desktop/background.hpp"
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

desktop_item_t::desktop_item_t(background_t* background, std::string title, std::string program, std::string icon)
{
	this->background = background;
	this->title = title;
	this->program = program;
	this->icon = icon;

	label = new label_t();
	this->addChild(label);

	label->setTitle(title);
	label->setColor(ARGB(255, 255, 255, 255));
	label->setBounds(g_rectangle(0, 0, 100, 100));
	surface = cairo_image_surface_create_from_png(this->icon.c_str());
}

void desktop_item_t::layout()
{
	g_rectangle bounds = getBounds();
	int padding = 10;
	int labelHeight = 20;
	label->setBounds(g_rectangle(padding, bounds.height - labelHeight - 5, bounds.width - padding * 2, labelHeight));
	label->setAlignment(g_text_alignment::CENTER);
}

void desktop_item_t::paint()
{
	cairo_t* cr = graphics.getContext();
	auto bounds = getBounds();

	cairo_save(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);

	auto selected = (background->getSelectedItem() == this);
	if(hovered || selected)
	{
		roundedRectangle(cr, 3, 3, bounds.width - 6, bounds.height - 6, 5);
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, selected ? 0.5 : 0.3);
		cairo_fill(cr);
	}

	if(surface)
	{
		int imgwidth = cairo_image_surface_get_width(surface);
		int imgheight = cairo_image_surface_get_height(surface);
		int imgx = bounds.width / 2 - imgwidth / 2;
		int imgy = 10;
		cairo_set_source_surface(cr, surface, imgx, imgy);
		cairo_rectangle(cr, imgx, imgy, bounds.width, bounds.height);
		cairo_fill(cr);
	}
}

void spawnerThread(desktop_item_t* item)
{
	g_spawn(item->program.c_str(), "", "", G_SECURITY_LEVEL_APPLICATION);
}

component_t* desktop_item_t::handleMouseEvent(mouse_event_t& e)
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
		background->setSelectedItem(this);
		markFor(COMPONENT_REQUIREMENT_PAINT);

		pressOffset = e.position;
		pressLocation = this->getBounds().getStart();

		if(e.clickCount >= 2)
		{
			// TODO
			g_create_thread_d((void*) &spawnerThread, this);
		}
	}
	else if(e.type == G_MOUSE_EVENT_DRAG)
	{
		auto bounds = this->getBounds();
		bounds.x += e.position.x - pressOffset.x;
		bounds.y += e.position.y - pressOffset.y;
		this->setBounds(bounds);
	}
	else if(e.type == G_MOUSE_EVENT_DRAG_RELEASE)
	{
		auto bounds = this->getBounds();
		int gridScale = this->background->gridScale;
		bounds.x += gridScale / 2;
		bounds.x = bounds.x - bounds.x % gridScale;
		bounds.y += gridScale / 2;
		bounds.y = bounds.y - bounds.y % gridScale;
		this->setBounds(bounds);
	}

	return this;
}