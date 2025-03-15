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

#include "list.hpp"

#include <ghost.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>

#include <libterminal/terminal.hpp>

int countTasks()
{
	g_kernquery_task_count_data out;
	g_kernquery_status countstatus = g_kernquery(G_KERNQUERY_TASK_COUNT, (uint8_t*) &out);
	if(countstatus != G_KERNQUERY_STATUS_SUCCESSFUL)
	{
		fprintf(stderr, "failed to query the kernel for the number of tasks (code %i)\n", countstatus);
		return -1;
	}
	return out.count;
}

bool getTaskIds(uint32_t initialBufSize, g_tid** out_ids, uint32_t* out_count)
{

	g_kernquery_task_list_data data;
	data.id_buffer = new g_tid[initialBufSize];
	data.id_buffer_size = initialBufSize;
	g_kernquery_status liststatus = g_kernquery(G_KERNQUERY_TASK_LIST, (uint8_t*) &data);
	if(liststatus != G_KERNQUERY_STATUS_SUCCESSFUL)
	{
		fprintf(stderr, "failed to query the kernel for the list of task ids (code %i)\n", liststatus);
		return false;
	}

	*out_ids = data.id_buffer;
	*out_count = data.filled_ids;
	return true;
}

int procListCompareByParent(const void* a, const void* b)
{
	g_kernquery_task_get_data* dataA = ((g_kernquery_task_get_data*) a);
	g_kernquery_task_get_data* dataB = ((g_kernquery_task_get_data*) b);

	if(dataA->parent == dataB->parent)
		return 0;
	else if(dataA->parent < dataB->parent)
		return -1;
	else
		return 1;
}


std::unordered_map<g_tid, uint64_t> lastCpuTimes;

/**
 *
 */
int procList(int argc, char** argv, bool top)
{
	bool threads = false;
	for(int i = 0; i < argc; i++)
	{
		if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0)
			threads = true;
	}

	int numTasks = countTasks();
	if(numTasks == -1)
		return -1;

	// get list of task ids
	g_tid* ids;
	uint32_t taskCount;
	if(!getTaskIds(numTasks + 16, &ids, &taskCount))
		return -1;

	// get detail information for each task
	g_kernquery_task_get_data* taskData = new g_kernquery_task_get_data[taskCount];
	for(uint32_t i = 0; i < taskCount; i++)
	{
		taskData[i].id = ids[i];
		g_kernquery_status getstatus = g_kernquery(G_KERNQUERY_TASK_GET_BY_ID, (uint8_t*) &taskData[i]);
		if(getstatus != G_KERNQUERY_STATUS_SUCCESSFUL)
		{
			continue;
		}
	}

	qsort(taskData, taskCount, sizeof(g_kernquery_task_get_data), procListCompareByParent);

	// print information
	if(top)
	{
		g_terminal::clear();
		g_terminal::setCursor(g_term_cursor_position(0, 0));
	}

	println("%4s %4s %-20s %-38s %5s %5s", "pid", "tid", "id", "path", "mem", top ? "cpu" : "");
	for(uint32_t pos = 0; pos < taskCount; pos++)
	{
		g_kernquery_task_get_data* entry = &taskData[pos];

		if(entry->id != -1 && (entry->type == G_TASK_TYPE_DEFAULT || entry->type == G_TASK_TYPE_VM86) && (
			   threads || entry->id == entry->parent))
		{
			if(top)
			{
				uint64_t cpuTimeTaken = entry->cpu_time - lastCpuTimes[entry->id];
				lastCpuTimes[entry->id] = entry->cpu_time;;

				println("%4i %4i %-20s %-38s %5i %5i",
				        entry->parent,
				        entry->id,
				        entry->identifier,
				        entry->source_path,
				        entry->memory_used / 1024,
				        cpuTimeTaken / 1000000);
			}
			else
			{
				println("%4i %4i %-20s %-38s %5i",
				        entry->parent,
				        entry->id,
				        entry->identifier,
				        entry->source_path,
				        entry->memory_used / 1024);
			}
		}
	}

	return 0;
}
