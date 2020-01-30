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

#include <stdio.h>
#include <ghostuser/utils/local.hpp>
#include <ghost/kernquery.h>

/**
 *
 */
int countTasks() {
	g_kernquery_task_count_data out;
	g_kernquery_status countstatus = g_kernquery(G_KERNQUERY_TASK_COUNT, (uint8_t*) &out);
	if (countstatus != G_KERNQUERY_STATUS_SUCCESSFUL) {
		fprintf(stderr, "failed to query the kernel for the number of tasks (code %i)\n", countstatus);
		return -1;
	}
	return out.count;
}

/**
 *
 */
bool getTaskIds(uint32_t initialBufSize, g_tid** out_ids, uint32_t* out_count) {

	g_kernquery_task_list_data data;
	data.id_buffer = new g_tid[initialBufSize];
	data.id_buffer_size = initialBufSize;
	g_kernquery_status liststatus = g_kernquery(G_KERNQUERY_TASK_LIST, (uint8_t*) &data);
	if (liststatus != G_KERNQUERY_STATUS_SUCCESSFUL) {
		fprintf(stderr, "failed to query the kernel for the list of task ids (code %i)\n", liststatus);
		return false;
	}

	*out_ids = data.id_buffer;
	*out_count = data.filled_ids;
	return true;
}

/**
 *
 */
int proc_list(int argc, char** argv) {

	// get number of tasks in the system
	int numTasks = countTasks();
	if (numTasks == -1) {
		return -1;
	}

	// get list of task ids
	int initialBufSize = numTasks + 10;
	g_tid* ids;
	uint32_t tasksFound;
	getTaskIds(initialBufSize, &ids, &tasksFound);

	// get detail information for each task
	g_kernquery_task_get_data* infos = new g_kernquery_task_get_data[tasksFound];
	for (uint32_t i = 0; i < tasksFound; i++) {
		g_local<g_kernquery_task_get_data> out(new g_kernquery_task_get_data());
		out()->id = ids[i];
		g_kernquery_status getstatus = g_kernquery(G_KERNQUERY_TASK_GET_BY_ID, (uint8_t*) out());
		if (getstatus != G_KERNQUERY_STATUS_SUCCESSFUL) {
			fprintf(stderr, "failed to query the kernel for task %i (code %i)\n", i, getstatus);
			continue;
		}

		infos[i] = *(out());
	}

	// print information
	println("%5s %5s %6s %-20s %-38s", "TID", "PID", "MEM", "NAME", "PATH");
	for (uint32_t pos = 0; pos < tasksFound; pos++) {
		g_kernquery_task_get_data* entry = &infos[pos];

		// print processes and vm86
		if (entry->type == G_THREAD_TYPE_MAIN || entry->type == G_THREAD_TYPE_VM86) {

			// print foot line
			println("%5i %5i %6i %-20s %-38s", entry->id, entry->parent, entry->memory_used / 1024, entry->identifier, entry->source_path);
		}
	}

	return 0;
}
