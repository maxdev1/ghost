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

#include "malloc.h"
#include "stdio.h"
#include "stdio_internal.h"

/**
 *
 */
uint8_t* ftostr(const char* filename)
{
	FILE* file = fopen(filename, "r");
	if(!file)
		return NULL;

	fseek(file, 0L, SEEK_END);
	off_t total = ftell(file);
	if(total < 0)
	{
		fclose(file);
		return NULL;
	}

	fseek(file, 0L, SEEK_SET);

	uint8_t* buf = malloc(total + 1);
	if(!buf)
	{
		fclose(file);
		return NULL;
	}

	off_t done = 0;
	off_t read;
	while((read = fread(&buf[done], 1, total - done, file)) > 0)
	{
		done += read;
	}
	fclose(file);

	if(done != total)
	{
		free(buf);
		return NULL;
	}

	buf[total] = 0;
	return buf;
}
