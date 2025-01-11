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

#include "components/desktop/background.hpp"
#include "components/desktop/desktop_item.hpp"

background_t::background_t()
{
	selection = new selection_t();
	selection->setVisible(false);
	addChild(selection);
}

void background_t::load(const char* path)
{
	if(surface)
		cairo_surface_destroy(surface);

	surface = cairo_image_surface_create_from_png(path);
}

void background_t::paint()
{
	cairo_t* cr = graphics.getContext();
    if(!cr) return;

	auto bounds = getBounds();

	// Background pattern
	cairo_pattern_t* gradient = cairo_pattern_create_linear(bounds.width * 0.4, 0, bounds.width * 0.8, bounds.height);
	cairo_pattern_add_color_stop_rgb(gradient, 0.0, 105.0 / 255.0, 84.0 / 255.0, 161.0 / 255.0);
	cairo_pattern_add_color_stop_rgb(gradient, 1.0, 22.0 / 255.0, 50.0 / 255.0, 100.0 / 255.0);
	cairo_rectangle(cr, 0, 0, bounds.width, bounds.height);
	cairo_set_source(cr, gradient);
	cairo_fill(cr);
	cairo_pattern_destroy(gradient);

	// Background image
	if(surface)
	{
		int imgwidth = cairo_image_surface_get_width(surface);
		int imgheight = cairo_image_surface_get_height(surface);

		int bgx = bounds.x + (bounds.width / 2 - imgwidth / 2);
		int bgy = bounds.y + (bounds.height / 2 - imgheight / 2);
		cairo_set_source_surface(cr, surface, bgx, bgy);
		cairo_rectangle(cr, bgx, bgy, bounds.width, bounds.height);
		cairo_fill(cr);
	}
}

void background_t::showSelection(g_rectangle& newSelection)
{
	if(newSelection.width != 0 || newSelection.height != 0)
	{
		selection->setBounds(newSelection.asNormalized());
		selection->setVisible(true);
	}
	else
	{
		selection->setVisible(false);
	}

	if(this->selectedItem)
	{
		auto previouslySelected = this->selectedItem;
		this->selectedItem = nullptr;
		previouslySelected->markFor(COMPONENT_REQUIREMENT_PAINT);
	}
}

void background_t::hideSelection()
{
	selection->setVisible(false);
}

void background_t::startLoadDesktopItems()
{
	desktop_item_t* terminal = new desktop_item_t(this, "Terminal", "/applications/terminal.bin", "/system/graphics/app-icons/terminal.png");
	terminal->setBounds(g_rectangle(gridScale, gridScale, gridScale, gridScale));
	this->addChild(terminal);

	desktop_item_t* calculator = new desktop_item_t(this, "Calculator", "/applications/calculator.bin", "/system/graphics/app-icons/calculator.png");
	calculator->setBounds(g_rectangle(gridScale, gridScale * 2, gridScale, gridScale));
	this->addChild(calculator);
}

void background_t::setSelectedItem(desktop_item_t* item)
{
	auto previouslySelected = this->selectedItem;
	this->selectedItem = item;
	if(previouslySelected)
		previouslySelected->markFor(COMPONENT_REQUIREMENT_PAINT);
}

desktop_item_t* background_t::getSelectedItem()
{
	return this->selectedItem;
}