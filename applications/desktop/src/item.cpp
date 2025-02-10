/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "item.hpp"
#include <cairo/cairo.h>
#include <math.h>
#include <libfont/text_alignment.hpp>
#include <libwindow/color_argb.hpp>
#include <helper.hpp>

item::item(uint32_t id):
	g_component(id),
	g_canvas(id)
{
	this->setBufferListener([this]()
	{
		this->paint();
	});
}

item* item::create(std::string name, std::string icon, std::string application)
{
	auto instance = createCanvasComponent<item>();
	instance->init(name, icon, application);
	return instance;
}

void item::init(std::string name, std::string icon, std::string application)
{
	this->application = application;
	iconSurface = cairo_image_surface_create_from_png(icon.c_str());
	if(cairo_surface_status(iconSurface) != CAIRO_STATUS_SUCCESS)
	{
		klog("failed to load image: %s", icon.c_str());
		iconSurface = nullptr;
	}

	label = g_label::create();
	label->setTitle(name.c_str());
	label->setBounds(g_rectangle(0, 75, 100, 20));
	label->setColor(ARGB(255, 255, 255, 255));
	label->setAlignment(g_text_alignment::CENTER);
	this->addChild(label);
}

void item::paint()
{
	auto cr = this->acquireGraphics();
	if(!cr)
		return;

	auto bounds = this->getBounds();

	cairo_save(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);

	if(hover || selected)
	{
		cairo_save(cr);
		cairo_rounded_rectangle(cr, 3, 3, bounds.width - 6, bounds.height - 6, 5);
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, selected ? 0.5 : 0.3);
		cairo_fill(cr);
		cairo_restore(cr);
	}

	if(iconSurface)
	{
		cairo_save(cr);
		int iconWidth = cairo_image_surface_get_width(iconSurface);
		int iconX = bounds.width / 2 - iconWidth / 2;
		int iconY = 10;
		cairo_set_source_surface(cr, iconSurface, iconX, iconY);
		cairo_rectangle(cr, iconX, iconY, bounds.width, bounds.height);
		cairo_fill(cr);
		cairo_restore(cr);
	}

	this->releaseGraphics();
	this->blit(g_rectangle(0, 0, bounds.width, bounds.height));
}

void item::onDoubleClick()
{
	klog(("Launching: " + this->application).c_str());
	g_spawn(this->application.c_str(), "", "", G_SECURITY_LEVEL_APPLICATION);
}
