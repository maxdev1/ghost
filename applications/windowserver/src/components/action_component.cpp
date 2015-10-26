/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include <components/action_component.hpp>
#include <stdio.h>

/**
 *
 */
void action_component_t::setActionListener(g_tid target_thread, g_ui_component_id component_id) {
	this->target_thread = target_thread;
	this->component_id = component_id;
}

/**
 *
 */
void action_component_t::fireAction() {

	if (this->target_thread != -1) {
		g_ui_component_action_event action_event;
		action_event.header.type = G_UI_COMPONENT_EVENT_TYPE_ACTION;
		action_event.header.component_id = this->component_id;
		g_send_message(this->target_thread, &action_event, sizeof(g_ui_component_action_event));
	}
}
