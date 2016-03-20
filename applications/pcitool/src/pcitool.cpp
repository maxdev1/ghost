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
#include <string.h>

#define MAJOR	0
#define MINOR	1

#include "list/list.hpp"

/**
 *
 */
int main(int argc, char** argv) {

	if (argc > 1) {
		char* command = argv[1];

		if (strcmp(command, "list") == 0) {
			return pci_list();

		} else if(strcmp(command, "--help") == 0) {
			println("pcitool, v%i.%i", MAJOR, MINOR);
			println("This program allows users to list data about PCI devices in the system.");
			println("");
			println("The following commands are available:");
			println("");
			println("\tlist\t\tlists basic information for all PCI devices");
			println("");

		} else {
			fprintf(stderr, "unknown command: %s\n", command);
		}
	} else {
		fprintf(stderr, "usage:\n\t%s <command>\n", argv[0]);
		fprintf(stderr, "Type \"%s --help\" for a list of commands.\n", argv[0]);
		fprintf(stderr, "\n");
	}
}
