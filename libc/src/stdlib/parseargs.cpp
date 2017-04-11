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
#include "ctype.h"
#include "stdio.h"
#include "string.h"
#include "libgen.h"

/**
 *
 */
char* get_executable_name() {
	// read path for the executable
	char* absoluteExecPath = (char*) malloc(G_PATH_MAX);
	g_get_executable_path(absoluteExecPath);

	// find base name
	char* execBaseName = basename(absoluteExecPath);

	// copy base name into smaller buffer
	char* execName = 0;

	size_t len = strlen(execBaseName);
	if (len < G_PATH_MAX) {
		execName = (char*) malloc(len + 1);
		strcpy(execName, execBaseName);
	}

	// free the absolute buffer
	free(absoluteExecPath);

	return execName;
}

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

	// count arguments
	char* pos = unparsedCommandLine;
	int argc = 1;
	while (*pos) {

		// read until unit separator
		while(*pos) {
			if(*pos == G_CLIARGS_SEPARATOR) {
				pos++;
				break;
			}
			pos++;
		}

		++argc;
	}

	// create argument array
	char** argv = (char**) malloc(sizeof(char*) * (argc + 1));
	if (argv == NULL) {
		return -1;
	}

	// put executable path as first argument
	argv[0] = get_executable_name();

	// parse command line
	pos = unparsedCommandLine;
	int argpp = 1;
	while (*pos) {
		argv[argpp++] = pos;

		// skip until unit separator
		while(*pos) {
			if(*pos == G_CLIARGS_SEPARATOR) {
				break;
			}
			pos++;
		}

		// no more chars - break
		if (*pos == 0) {
			break;
		}

		*pos++ = 0;
	}

	// mark end of argument vector
	argv[argpp] = 0;

	// clean up arguments
	for (int i = 1; i < argc; i++) {
		char* arg = argv[i];
		size_t arglen = strlen(arg);

		bool esc = false;
		for (int p = 0; p < arglen; p++) {

			if (!esc && arg[p] == '\\') {
				for (int x = p; x < arglen; x++) {
					arg[x] = arg[x + 1];
				}
				arglen--;
				--p; // repeat on same position
				esc = true;

			} else if (!esc && arg[p] == '"') {
				for (int x = p; x < arglen; x++) {
					arg[x] = arg[x + 1];
				}
				arglen--;
				--p; // repeat on same position
				esc = false;

			} else {
				esc = false;
			}
		}
	}

	// write out parameters
	*out_argc = argc;
	*out_args = argv;

	return 0;
}
