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

#include <stdio.h>
#include <ghost.h>

/**
 *
 */
int main(int argc, char** argv) {

	g_fs_open_directory_status stat;
	char* workdir = new char[G_PATH_MAX];
	g_get_working_directory(workdir);
	auto* iter = g_open_directory_s(workdir, &stat);

	if (stat != G_FS_OPEN_DIRECTORY_SUCCESSFUL) {
		fprintf(stderr, "failed to read directory\n");
		return 1;
	}

	g_fs_directory_entry* entry;
	while (true) {
		g_fs_read_directory_status rstat;
		entry = g_read_directory_s(iter, &rstat);

		if (rstat == G_FS_READ_DIRECTORY_SUCCESSFUL) {
			if (entry == 0) {
				break;
			}

			if (entry->type == G_FS_NODE_TYPE_FILE) {
				printf("   ");
			} else if (entry->type == G_FS_NODE_TYPE_FOLDER) {
				printf(" + ");
			} else {
				printf(" ~ ");
			}
			printf("%s\n", entry->name);
		} else {
			break;
		}
	}

	g_close_directory(iter);

}
