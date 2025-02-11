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

#include "components/desktop/screen.hpp"
#include "components/window.hpp"

#include <cstring>

void screen_t::addChild(component_t* comp, component_child_reference_type_t type)
{
	component_t::addChild(comp, type);

	if(comp->isWindow())
	{
		this->callForListeners(G_UI_COMPONENT_EVENT_TYPE_WINDOWS, [this, comp](event_listener_info_t& info)
		{
			auto window = dynamic_cast<window_t*>(comp);
			sendWindowEvent(info.component_id, window, info.target_thread, true);
			window->onTitleChanged([this, info, window]()
			{
				sendWindowEvent(info.component_id, window, info.target_thread, true);
			});
		});
	}
}

void screen_t::removeChild(component_t* comp)
{
	if(comp->isWindow())
	{
		this->callForListeners(G_UI_COMPONENT_EVENT_TYPE_WINDOWS, [this, comp](event_listener_info_t& info)
		{
			sendWindowEvent(info.component_id, dynamic_cast<window_t*>(comp), info.target_thread,
			                false);
		});
	}

	component_t::removeChild(comp);
}

void screen_t::sendWindowEvent(g_ui_component_id observerId, window_t* window, g_tid observerThread, bool present)
{
	g_ui_windows_event windowEvent;
	windowEvent.header.type = G_UI_COMPONENT_EVENT_TYPE_WINDOWS;
	windowEvent.header.component_id = observerId;
	windowEvent.window_id = window->id;
	windowEvent.present = present;
	g_send_message(observerThread, &windowEvent, sizeof(g_ui_windows_event));
}

void screen_t::markDirty(g_rectangle rect)
{
	g_mutex_acquire(lock);

	if(invalid.width == 0 && invalid.height == 0)
	{
		invalid = rect;
	}
	else
	{
		invalid.extend(rect.getStart());
		invalid.extend(rect.getEnd());
	}

	// Fix invalid area
	if(invalid.x < 0)
	{
		invalid.width += invalid.x;
		invalid.x = 0;
	}
	if(invalid.y < 0)
	{
		invalid.height += invalid.y;
		invalid.y = 0;
	}
	if(invalid.x + invalid.width > getBounds().width)
	{
		invalid.width = getBounds().width - invalid.x;
	}
	if(invalid.y + invalid.height > getBounds().height)
	{
		invalid.height = getBounds().height - invalid.y;
	}
	g_mutex_release(lock);
}

g_rectangle screen_t::grabInvalid()
{
	g_mutex_acquire(lock);
	g_rectangle ret = invalid;
	invalid = g_rectangle();
	g_mutex_release(lock);
	return ret;
}
