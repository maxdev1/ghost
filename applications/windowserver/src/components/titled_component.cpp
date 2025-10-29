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

#include "titled_component.hpp"
#include "event_listener_info.hpp"
#include <cstring>

void titled_component_t::setTitle(std::string title)
{
	setTitleInternal(title);

	this->callForListeners(G_UI_COMPONENT_EVENT_TYPE_TITLE, [title](event_listener_info_t& info)
	{
		auto event = new g_ui_component_title_event;
		event->header.type = G_UI_COMPONENT_EVENT_TYPE_TITLE;
		event->header.component_id = info.component_id;
		strncpy(event->title, title.c_str(), G_UI_COMPONENT_TITLE_MAXIMUM);
		platformSendMessage(info.target_thread, event, sizeof(g_ui_component_title_event), SYS_TX_NONE);
		delete event;
	});
}
