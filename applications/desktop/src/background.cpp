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

#include "background.hpp"
#include <cairo/cairo.h>
#include <stdlib.h>

background_t* background_t::create()
{
	auto instance = createCanvasComponent<background_t>();
	instance->init();
	return instance;
}

void background_t::init()
{
	organizer = new item_organizer_t(100, 100, 5, 5);

	this->setBufferListener([this]()
	{
		paint();
	});

	this->addMouseListener([this](g_ui_component_mouse_event* e)
	{
		auto position = e->position;
		if(e->type == G_MOUSE_EVENT_MOVE)
		{
			onMouseMove(position);
		}
		else if(e->type == G_MOUSE_EVENT_PRESS && e->buttons & G_MOUSE_BUTTON_1)
		{
			onMouseLeftPress(position, e->clickCount);
		}
		else if(e->type == G_MOUSE_EVENT_DRAG && e->buttons & G_MOUSE_BUTTON_1)
		{
			onMouseDrag(position);
		}
		else if(e->type == G_MOUSE_EVENT_RELEASE)
		{
			onMouseRelease(position);
		}
		else if(e->type == G_MOUSE_EVENT_LEAVE)
		{
			onMouseRelease(position);
		}
	});

	selection = g_selection::create();
	selection->setBounds(this->getBounds());
	selection->setVisible(false);
	this->addChild(selection);

	this->organize();
}

void background_t::onMouseMove(const g_point& position)
{
	for(auto item: items)
	{
		auto rect = item->getBounds();
		bool oldHover = item->hover;
		item->hover = rect.contains(position);

		if(item->hover != oldHover)
			item->paint();
	}
}

void background_t::onMouseLeftPress(const g_point& position, int clickCount)
{
	item_t* pressedItem = nullptr;
	std::vector<g_rectangle> itemsBounds;
	for(auto& item: items)
	{
		auto itemBounds = item->getBounds();
		itemsBounds.push_back(itemBounds);

		if(itemBounds.contains(position))
		{
			pressedItem = item;
		}
	}

	if(pressedItem)
	{
		if(clickCount == 2)
		{
			pressedItem->onDoubleClick();
		}

		// If pressed item not selected, unselect all others
		if(!pressedItem->selected)
		{
			for(auto& item: items)
			{
				auto wasSelected = item->selected;
				item->selected = false;
				if(wasSelected != item->selected)
					item->paint();
			}

			auto wasSelected = pressedItem->selected;
			pressedItem->selected = true;
			if(wasSelected != pressedItem->selected)
				pressedItem->paint();
		}

		// Store for each selected item the offset in relation to press point
		for(int i = 0; i < items.size(); i++)
		{
			if(items[i]->selected)
			{
				items[i]->dragOffset = position - itemsBounds[i].getStart();
			}
		}

		dragItems = true;
	}
	else
	{
		for(auto item: items)
		{
			auto wasSelected = item->selected;
			item->selected = false;
			if(wasSelected != item->selected)
				item->paint();
		}

		selectionStart = position;
	}
}

void background_t::onMouseDrag(const g_point& position)
{
	if(dragItems)
	{
		for(auto item: items)
		{
			if(item->selected)
			{
				auto point = position - item->dragOffset;
				item->setBounds(g_rectangle(point.x, point.y, 100, 100));
			}
		}
	}
	else
	{
		g_rectangle bounds(selectionStart.x, selectionStart.y, position.x - selectionStart.x,
		                   position.y - selectionStart.y);
		auto boundsNorm = bounds.asNormalized();
		selection->setBounds(boundsNorm);
		selection->setVisible(true);

		for(auto item: items)
		{
			g_rectangle rect = item->getBounds();
			if(boundsNorm.intersects(rect))
			{
				auto wasSelected = item->selected;
				item->selected = true;
				if(wasSelected != item->selected)
					item->paint();
			}
			else
			{
				auto wasSelected = item->selected;
				item->selected = false;
				if(wasSelected != item->selected)
					item->paint();
			}
		}
	}
}

void background_t::onMouseRelease(const g_point& position)
{
	if(dragItems)
		dragItems = false;
	selection->setVisible(false);
	organize();
}

void background_t::organize()
{
	this->organizer->organize(items, this->getBounds());
}

void background_t::addTaskbar(taskbar_t* taskbar)
{
	this->taskbar = taskbar;
	this->addChild(taskbar);
}


void background_t::paint()
{
	auto cr = this->acquireGraphics();
	if(!cr)
	{
		return;
	}

	auto bounds = this->getBounds();

	srand(g_millis());
	static int r = rand() % 100 + 50;

	cairo_pattern_t* gradient = cairo_pattern_create_linear(bounds.width * 0.4, 0, bounds.width * 0.8,
	                                                        bounds.height);
	cairo_pattern_add_color_stop_rgb(gradient, 0.0, /*105.0*/r / 255.0, 84.0 / 255.0, 161.0 / 255.0);
	cairo_pattern_add_color_stop_rgb(gradient, 1.0, 22.0 / 255.0, 50.0 / 255.0, 100.0 / 255.0);
	cairo_rectangle(cr, 0, 0, bounds.width, bounds.height);
	cairo_set_source(cr, gradient);
	cairo_fill(cr);
	cairo_pattern_destroy(gradient);

	int imgWidth = bounds.width;
	int imgHeight = bounds.height;

	int bgX = bounds.x + (bounds.width / 2 - imgWidth / 2);
	int bgY = bounds.y + (bounds.height / 2 - imgHeight / 2);
	cairo_rectangle(cr, bgX, bgY, bounds.width, bounds.height);
	cairo_fill(cr);

	this->blit(g_rectangle(0, 0, bounds.width, bounds.height));

	this->releaseGraphics();
}

void background_t::addItem(item_t* item)
{
	items.push_back(item);
	this->addChild(item);
}
