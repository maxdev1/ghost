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

#include "tester.hpp"
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

/**
 *
 */
int main(int argc, char** argv) {

	if (argc > 1) {
		char* dirbuf = strdup(argv[1]);
		char* basebuf = strdup(argv[1]);
		printf("basename: %s\n", dirname(dirbuf));
		printf("dirname:  %s\n", basename(basebuf));
		free(dirbuf);
		free(basebuf);
	} else {
		fprintf(stderr, "specify a path name");
	}

}
