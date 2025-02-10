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

#include "components/focusable_component.hpp"
#include "components/component.hpp"

component_t* focusable_component_t::setFocused(bool focused)
{
	if(isFocusable())
	{
		setFocusedInternal(focused);

		self->callForListeners(G_UI_COMPONENT_EVENT_TYPE_FOCUS, [focused](event_listener_info_t& info)
		{
			g_ui_component_focus_event event;
			event.header.type = G_UI_COMPONENT_EVENT_TYPE_FOCUS;
			event.header.component_id = info.component_id;
			event.now_focused = focused;
			g_send_message(info.target_thread, &event, sizeof(g_ui_component_focus_event));
		});

		return self;
	}

	auto parent = self->getParent();
	if(parent)
		return parent->setFocused(focused);
	return nullptr;
}
