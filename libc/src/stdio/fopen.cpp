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

#include "stdio.h"
#include "stdio_internal.h"
#include "stdint.h"
#include "file.h"
#include "unistd.h"
#include "errno.h"
#include "malloc.h"

/**
 *
 */
FILE* fopen(const char* filename, const char* mode) {

	// allocate the file structure
	FILE* file = (FILE*) calloc(sizeof(FILE), 1);
	if (file == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	// static-open it
	FILE* res = __fopen_static(filename, mode, file);
	if (res == NULL) {
		// if it failed, free the file
		free(file);
		return NULL;
	}

	return res;
}
