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

#include "stdlib.h"
#include "malloc.h"
#include "ghost.h"

/**
 * Prepares command line arguments
 */
int parseargs(int* out_argc, char*** out_args) {

	// get argument line from system
	char* unparsedCommandLine = (char*) malloc(G_CLIARGS_BUFFER_LENGTH);
	if (unparsedCommandLine == NULL) {
		return -1;
	}
	g_cli_args_release(unparsedCommandLine);

	// TODO implement parsing, these are dummy arguments
	char** args = (char**) malloc(sizeof(char*) * 3);
	if (args == NULL) {
		return -1;
	}

	args[0] = (char*) "program";
	args[1] = unparsedCommandLine;
	args[2] = 0;

	*out_argc = 2;
	*out_args = args;

	return 0;
}
