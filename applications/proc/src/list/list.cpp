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
#include <ghost/utils/local.hpp>
#include <ghost/kernquery.h>

/**
 *
 */
int proc_list(int argc, char** argv) {

	// read how many devices there are
	g_kernquery_task_count_out out;

	g_kernquery_status countstatus = g_kernquery(G_KERNQUERY_TASK_COUNT, 0, (uint8_t*) &out);
	if (countstatus != G_KERNQUERY_STATUS_SUCCESSFUL) {
		fprintf(stderr, "failed to query the kernel for the number of tasks (code %i)\n", countstatus);
		return 1;
	}

	// list tasks
	g_kernquery_task_get_out* infos = new g_kernquery_task_get_out[out.count];

	for (uint32_t pos = 0; pos < out.count; pos++) {
		// query task at position
		g_kernquery_task_get_by_pos_in in;
		in.position = pos;

		g_local<g_kernquery_task_get_out> out(new g_kernquery_task_get_out());

		g_kernquery_status getstatus = g_kernquery(G_KERNQUERY_TASK_GET_BY_POS, (uint8_t*) &in, (uint8_t*) out());
		if (getstatus != G_KERNQUERY_STATUS_SUCCESSFUL) {
			fprintf(stderr, "failed to query the kernel for task at position %i (code %i)", pos, getstatus);
			return 1;
		}

		infos[pos] = *(out());
	}

	// print information
	println("   %-4s %-30s %-12s %-8s %-7s", "TID", "SOURCE", "IDENTIFIER", "TYPE", "MEM(kb)");
	for (uint32_t pos = 0; pos < out.count; pos++) {
		g_kernquery_task_get_out* entry = &infos[pos];

		// print processes and vm86
		if (entry->type == G_THREAD_TYPE_MAIN || entry->type == G_THREAD_TYPE_VM86) {

			const char* textType = "?";
			if (entry->type == G_THREAD_TYPE_MAIN) {
				textType = "normal";
			} else if (entry->type == G_THREAD_TYPE_VM86) {
				textType = "vm86";
			}

			// check if it has threads
			bool hasThreads = false;
			for (uint32_t pos = 0; pos < out.count; pos++) {
				g_kernquery_task_get_out* subentry = &infos[pos];
				if (subentry->type == G_THREAD_TYPE_SUB && subentry->parent == entry->id) {
					hasThreads = true;
				}
			}

			// print foot line
			printf(" ");
			printf(hasThreads ? "\xC2" : "\xC4");
			println(" %-4i %-30s %-12s %-8s %-7i", entry->id, entry->source_path, entry->identifier, textType, entry->memory_used / 1024);

			// print subtasks
			for (uint32_t pos = 0; pos < out.count; pos++) {
				g_kernquery_task_get_out* subentry = &infos[pos];

				// check if there are more threads after this one
				bool moreFollow = false;
				for (uint32_t pos2 = pos + 1; pos2 < out.count; pos2++) {
					g_kernquery_task_get_out* subfollow = &infos[pos2];
					if (subfollow->type == G_THREAD_TYPE_SUB && subfollow->parent == entry->id) {
						moreFollow = true;
					}
				}

				if (subentry->type == G_THREAD_TYPE_SUB && subentry->parent == entry->id) {
					printf(" ");
					printf(moreFollow ? "\xC3" : "\xC0");
					println(" %-4i %-30s %-12s", subentry->id, "", subentry->identifier);
				}
			}
		}
	}

	return 0;
}
