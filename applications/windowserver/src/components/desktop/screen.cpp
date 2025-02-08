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
		event_listener_info_t listenerInfo;
		if(this->getListener(G_UI_COMPONENT_EVENT_TYPE_WINDOWS, listenerInfo))
		{
			auto window = (window_t*) comp;
			sendWindowEvent(listenerInfo.component_id, window, listenerInfo.target_thread, true);
			window->onTitleChanged([listenerInfo, window, this]()
			{
				sendWindowEvent(listenerInfo.component_id, window, listenerInfo.target_thread, true);
			});
		}
	}
}

void screen_t::removeChild(component_t* comp)
{
	if(comp->isWindow())
	{
		event_listener_info_t listenerInfo;
		if(this->getListener(G_UI_COMPONENT_EVENT_TYPE_WINDOWS, listenerInfo))
		{
			sendWindowEvent(listenerInfo.component_id, (window_t*) comp, listenerInfo.target_thread, false);
		}
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

	auto title = window->getTitle();
	size_t titleLen;
	if(title.length() >= G_UI_COMPONENT_TITLE_MAXIMUM)
	{
		titleLen = G_UI_COMPONENT_TITLE_MAXIMUM - 1;
	}
	else
	{
		titleLen = title.length();
	}
	memcpy(windowEvent.title, title.c_str(), titleLen);
	windowEvent.title[titleLen] = 0;

	g_send_message(observerThread, &windowEvent, sizeof(g_ui_windows_event));
}

void screen_t::markDirty(g_rectangle rect)
{
	// Mark area as invalid
	if(invalid.x == 0 && invalid.y == 0 && invalid.width == 0 && invalid.height == 0)
	{
		invalid = rect;
	}
	else
	{
		int top = rect.getTop() < invalid.getTop() ? rect.getTop() : invalid.getTop();
		int left = rect.getLeft() < invalid.getLeft() ? rect.getLeft() : invalid.getLeft();
		int bottom = rect.getBottom() > invalid.getBottom() ? rect.getBottom() : invalid.getBottom();
		int right = rect.getRight() > invalid.getRight() ? rect.getRight() : invalid.getRight();

		invalid = g_rectangle(left, top, right - left + 1, bottom - top + 1);
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
}
