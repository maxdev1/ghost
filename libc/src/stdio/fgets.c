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

/**
 * Reads at most n - 1 bytes, including the first newline, and terminates the
 * result. EOF before any input and read errors return a null pointer.
 */
char* fgets(char* s, int n, FILE* stream)
{
	if(n <= 0)
		return NULL;
	char* p = s;
	while(n > 1)
	{
		int c = fgetc(stream);
		if(c == EOF)
		{
			if(p == s || ferror(stream))
				return NULL;
			break;
		}
		*p++ = c;
		--n;
		if(c == '\n')
			break;
	}
	*p = 0;
	return s;
}

