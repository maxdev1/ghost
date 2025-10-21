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

#include "kernel/calls/syscall_kernquery.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/utils/hashmap.hpp"
#include "kernel/utils/string.hpp"

void syscallKernQuery(g_task* task, g_syscall_kernquery* data)
{
	if(data->command == G_KERNQUERY_TASK_LIST)
	{
		auto out = (g_kernquery_task_list_data*) data->buffer;

		uint32_t rem = out->id_buffer_size;
		out->filled_ids = 0;
		auto iter = hashmapIteratorStart(taskGlobalMap);
		while(rem-- && hashmapIteratorHasNext(&iter))
		{
			auto next = hashmapIteratorNext(&iter)->value;
			out->id_buffer[out->filled_ids++] = next->id;
		}
		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
		hashmapIteratorEnd(&iter);
	}
	else if(data->command == G_KERNQUERY_TASK_COUNT)
	{
		auto out = (g_kernquery_task_count_data*) data->buffer;

		data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
		out->count = hashmapSize(taskGlobalMap);
	}
	else if(data->command == G_KERNQUERY_TASK_GET_BY_ID)
	{
		auto out = (g_kernquery_task_get_data*) data->buffer;

		g_task* target = taskingGetById(out->id);
		mutexAcquire(&target->lock);

		if(!target || target->status == G_TASK_STATUS_DEAD)
		{
			data->status = G_KERNQUERY_STATUS_UNKNOWN_ID;
			out->id = -1;
		}
		else
		{
			data->status = G_KERNQUERY_STATUS_SUCCESSFUL;
			out->found = true;
			out->id = target->id;
			out->parent = target->process->id;
			out->type = target->type;

			if(target->process->environment.executablePath)
				stringCopy(out->source_path, target->process->environment.executablePath);
			else
				out->source_path[0] = 0;

			const char* identifier = taskingDirectoryGetIdentifier(target->id);
			if(identifier)
				stringCopy(out->identifier, identifier);
			else
				out->identifier[0] = 0;

			out->cpu_time = target->statistics.timesScheduled;
			out->memory_used = 0; // TODO
		}

		mutexRelease(&target->lock);
	}
	else
	{
		data->status = G_KERNQUERY_STATUS_ERROR;
	}
}
