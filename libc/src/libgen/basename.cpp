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

#include "libgen.h"
#include "ghost.h"
#include "string.h"

/**
 *
 */
char* basename(char* path) {

	// return dot if null
	if (path == NULL) {
		return ".";
	}

	// get length of the path
	int len = strlen(path);

	// return dot if empty
	if (len == 0) {
		return ".";
	}

	// return slash in these two cases
	if ((len == 1 && path[0] == '/')
			|| (len == 2 && path[0] == '/' && path[1] == '/')) {
		return "/";
	}

	// find base name part
	char* slashLocation = strrchr(path, '/');

	if (slashLocation == NULL) {
		return path;
	}

	return slashLocation + 1;
}

