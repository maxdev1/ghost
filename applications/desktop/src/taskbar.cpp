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

g_user_mutex tasksLock = g_mutex_initialize();
std::map<g_ui_component_id, taskbar_entry*> tasks;

taskbar::taskbar(g_ui_component_id id):
	g_canvas(id)
{
}

taskbar* taskbar::create()
{
	auto instance = createCanvasComponent<taskbar>();
	instance->init();
	return instance;
}

void taskbar::init()
{
	this->setBufferListener([this]()
	{
		paint();
	});

	this->setMouseListener([this](g_ui_component_mouse_event* e)
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
	});
}

void taskbar::onMouseMove(const g_point& position)
{
}

void taskbar::onMouseLeftPress(const g_point& position, int clickCount)
{
}

void taskbar::onMouseDrag(const g_point& position)
{
}

void taskbar::onMouseRelease(const g_point& position)
{
}

void taskbar::paint()
{
	auto cr = this->acquireGraphics();
	if(!cr)
		return;

	auto bounds = this->getBounds();

	cairo_save(cr);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, 0, 0, bounds.width, bounds.height);
	cairo_fill(cr);
	cairo_restore(cr);

	g_mutex_acquire(tasksLock);
	if(tasks.size() > 0)
	{
		int maxPerTask = bounds.width / tasks.size();
		int pos = 0;
		int taskWidth = std::min(200, maxPerTask);
		for(auto& task: tasks)
		{
			cairo_save(cr);
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_rectangle(cr, pos++ * taskWidth, 0, taskWidth, bounds.height);
			cairo_fill(cr);
			cairo_restore(cr);
		}
	}
	g_mutex_release(tasksLock);

	this->blit(g_rectangle(0, 0, bounds.width, bounds.height));

	this->releaseGraphics();
}

void taskbar::handleDesktopEvent(g_ui_windows_event* event)
{
	g_mutex_acquire(tasksLock);

	if(event->present)
	{
		taskbar_entry* entry = nullptr;
		if(tasks.count(event->window_id) > 0)
		{
			entry = tasks[event->window_id];
		}
		else
		{
			entry = new taskbar_entry();
			tasks[event->window_id] = entry;
		}

		entry->title = std::string(event->title);
	}
	else if(tasks.count(event->window_id) > 0)
	{
		delete tasks[event->window_id];
		tasks.erase(event->window_id);
	}

	g_mutex_release(tasksLock);

	this->paint();
}
