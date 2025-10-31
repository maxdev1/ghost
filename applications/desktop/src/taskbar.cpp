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

#include "taskbar.hpp"

#include <cairo/cairo.h>
#include <cstdlib>
#include <map>
#include <libwindow/font/text_layouter.hpp>
#include <libwindow/font/font_loader.hpp>
#include <helper.hpp>

g_font* font = nullptr;

taskbar_t::taskbar_t(g_ui_component_id id):
	g_component(id), g_canvas(id), g_focusable_component(id)
{
	textLayoutBuffer = g_text_layouter::getInstance()->initializeBuffer();
}

taskbar_t* taskbar_t::create()
{
	auto instance = createCanvasComponent<taskbar_t>();
	instance->init();
	return instance;
}

void taskbar_t::init()
{
	font = g_font_loader::getDefault();
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
			onMouseLeave(position);
		}
	});

	this->setFocusable(false);
	this->setDispatchesFocus(false);
}

void taskbar_t::onMouseMove(const g_point& position)
{
	g_mutex_acquire(entriesLock);
	bool changes = false;
	for(auto& entry: entries)
	{
		bool oldHovered = entry->hovered;
		entry->hovered = entry->onView.contains(position);
		if(oldHovered != entry->hovered)
			changes = true;
	}
	if(changes)
		paint();
	g_mutex_release(entriesLock);
}

void taskbar_t::onMouseLeftPress(const g_point& position, int clickCount)
{
	g_mutex_acquire(entriesLock);
	for(auto& entry: entries)
	{
		if(entry->onView.contains(position))
		{
			bool visible = entry->window->isVisible();
			if(visible)
			{
				if(entry->window->isFocused())
					entry->window->setVisible(false);
				else
					entry->window->setFocused(true);
			}
			else
			{
				entry->window->setVisible(true);
				entry->window->setFocused(true);
			}
			break;
		}
	}
	paint();
	g_mutex_release(entriesLock);
}

void taskbar_t::onMouseDrag(const g_point& position)
{
}

void taskbar_t::onMouseRelease(const g_point& position)
{
}

void taskbar_t::onMouseLeave(const g_point& position)
{
	g_mutex_acquire(entriesLock);
	bool changes = false;
	for(auto& entry: entries)
	{
		bool oldHovered = entry->hovered;
		entry->hovered = false;
		if(oldHovered != entry->hovered)
			changes = true;
	}
	if(changes)
		paint();
	g_mutex_release(entriesLock);
}

void taskbar_t::paint()
{
	auto cr = this->acquireGraphics();
	if(!cr)
		return;

	auto bounds = this->getBounds();

	cairo_save(cr);
	cairo_set_source_rgb(cr, 38.0 / 255, 40.0 / 255, 46.0 / 255);
	cairo_rectangle(cr, 0, 0, bounds.width, bounds.height);
	cairo_fill(cr);
	cairo_restore(cr);

	cairo_save(cr);
	cairo_set_source_rgb(cr, 57.0 / 255, 60.0 / 255, 68.0 / 255);
	cairo_rectangle(cr, 0, 0, bounds.width, 2);
	cairo_fill(cr);
	cairo_restore(cr);

	int taskAreaStart = 100;
	int taskAreaEnd = 50;
	int taskAreaWidth = bounds.width - taskAreaStart - taskAreaEnd;

	g_mutex_acquire(entriesLock);
	if(entries.size() > 0)
	{
		int maxPerTask = taskAreaWidth / entries.size();
		int pos = 0;
		int taskWidth = std::min(200, maxPerTask);

		for(auto& entry: entries)
		{
			int entryLeft = taskAreaStart + pos++ * taskWidth;

			cairo_save(cr);
			if(entry->focused && entry->visible)
			{
				if(entry->hovered)
				{
					cairo_set_source_rgb(cr, 37.0 / 255, 32.0 / 255, 39.0 / 255);
				}
				else
				{
					cairo_set_source_rgb(cr, 27.0 / 255, 28.0 / 255, 33.0 / 255);
				}
			}
			else
			{
				if(entry->hovered)
				{
					cairo_set_source_rgb(cr, 52.0 / 255, 55.0 / 255, 63.0 / 255);
				}
				else
				{
					cairo_set_source_rgb(cr, 50.0 / 255, 52.0 / 255, 58.0 / 255);
				}
			}
			cairo_rounded_rectangle(cr, entryLeft + 2, 4, taskWidth - 4, bounds.height - 8, 3);
			cairo_fill(cr);
			cairo_restore(cr);

			entry->onView = g_rectangle(entryLeft + 2, 4, taskWidth - 4, bounds.height - 8);

			g_rectangle textArea(10, 0, taskWidth - 20, bounds.height - 10);
			g_text_layouter::getInstance()->layout(cr, entry->title.c_str(), font, 14, textArea, g_text_alignment::LEFT,
			                                       textLayoutBuffer);
			for(g_positioned_glyph& g: textLayoutBuffer->positions)
			{

				g_rectangle onView(g.position.x, g.position.y, g.size.width, g.size.height);

				cairo_save(cr);
				cairo_set_source_rgb(cr, 0.96, 0.98, 0.98);
				// TODO text layouter is buggy (need to subtract glyph positions):
				cairo_translate(cr, entryLeft + onView.x - g.glyph->x, onView.y - g.glyph->y + 10);
				cairo_glyph_path(cr, g.glyph, g.glyph_count);
				cairo_fill(cr);
				cairo_restore(cr);
			}
		}
	}
	g_mutex_release(entriesLock);

	this->blit(g_rectangle(0, 0, bounds.width, bounds.height));
	this->releaseGraphics();
}

void taskbar_t::handleDesktopEvent(g_ui_windows_event* event)
{
	g_mutex_acquire(entriesLock);

	taskbar_entry_t* entry = nullptr;
	for(auto& existing: entries)
	{
		if(existing->window->getId() == event->window_id)
		{
			entry = existing;
			break;
		}
	}

	if(event->present)
	{
		if(!entry)
		{
			entry = new taskbar_entry_t();
			entry->window = g_window::attach(event->window_id);
			entry->title = entry->window->getTitle();
			if(entry->title.length() == 0)
			{
				g_sleep(500);
				entry->title = entry->window->getTitle();
			}
			entry->hovered = false;
			entry->visible = entry->window->isVisible();
			entry->focused = entry->window->isFocused();
			entry->window->addTitleListener([this, entry](std::string title)
			{
				entry->title = title;
				this->paint();
			});
			entry->window->addFocusListener([this, entry](bool focused)
			{
				entry->focused = focused;
				this->paint();
			});
			entry->window->addVisibleListener([this,entry](bool visible)
			{
				entry->visible = visible;
				this->paint();
			});
			entries.push_back(entry);
		}
	}
	else if(entry)
	{
		entries.erase(std::find(entries.begin(), entries.end(), entry));
		delete entry;
	}

	g_mutex_release(entriesLock);

	this->paint();
}
