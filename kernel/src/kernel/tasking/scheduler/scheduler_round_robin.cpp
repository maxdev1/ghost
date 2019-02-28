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

#include "kernel/tasking/scheduler.hpp"

void schedulerSchedule(g_tasking_local* local)
{
	if(!local->current)
	{
		local->current = local->list->task;
		return;
	}

	// Find current task in list
	g_task_entry* entry = local->list;
	while(entry)
	{
		if(entry->task == local->current)
		{
			break;
		}
		entry = entry->next;
	}

	// Select next in list
	if(entry)
	{
		entry = entry->next;

		if(entry)
		{
			local->current = entry->task;
		} else
		{
			local->current = local->list->task;
		}
	} else
	{
		local->current = local->list->task;
	}
}
