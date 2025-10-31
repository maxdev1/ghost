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

#include "components/action_component.hpp"
#include "components/component.hpp"

void action_component_t::fireAction()
{
	if(internalHandler)
	{
		internalHandler();
		return;
	}

	// otherwise send message to registered thread
	this->callForListeners(G_UI_COMPONENT_EVENT_TYPE_ACTION, [](event_listener_info_t& info)
	{
		g_ui_component_action_event actionEvent;
		actionEvent.header.type = G_UI_COMPONENT_EVENT_TYPE_ACTION;
		actionEvent.header.component_id = info.component_id;
		platformSendMessage(info.target_thread, &actionEvent, sizeof(g_ui_component_action_event), SYS_TX_NONE);
	});
}
