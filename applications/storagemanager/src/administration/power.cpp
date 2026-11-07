/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "power.hpp"
#include "jobs.hpp"
#include <ghost/system.h>

static uint32_t adminPowerRequest = 0;
static bool adminPowerAcknowledged = false;

bool adminPowerIsQuiescing() { return adminPowerRequest != 0; }

void adminPowerHandleEvent(const g_message_header* message, const g_power_event* event)
{
	if(message->sender != G_TID_NONE || message->length != sizeof(*event))
		return;

	if(event->magic != G_POWER_EVENT_MAGIC || !event->request || event->action > G_POWER_REBOOT)
		return;

	if(event->type == G_POWER_EVENT_PREPARE && !adminPowerIsQuiescing())
	{
		adminPowerRequest = event->request;
		adminPowerAcknowledged = false;
	}
	else if(event->type == G_POWER_EVENT_CANCEL && event->request == adminPowerRequest)
	{
		adminPowerRequest = 0;
		adminPowerAcknowledged = false;
		g_power_acknowledge(event->request, 1);
	}
}

void adminPowerPoll(bool controlsPending)
{
	if(!adminPowerIsQuiescing() || adminPowerAcknowledged)
		return;

	bool ready;
	bool pending = adminJobHasPending(&ready) || controlsPending;

	if(ready && pending)
		return;

	if(g_power_acknowledge(adminPowerRequest, ready) == G_POWER_SUCCESSFUL)
		adminPowerAcknowledged = true;
}
