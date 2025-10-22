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

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "malloc.h"
#include <ghost/system.h>

/**
 *
 */
void kvlog(const char* message, va_list l)
{
	va_list lc;
	va_copy(lc, l);

	// First try
	uint32_t messageLen = strlen(message);
	uint32_t bufLen = messageLen * 4;

	char* buf = malloc(bufLen);
	if(!buf)
	{
		g_log("failed to allocate buffer for kernel logging");
		return;
	}

	int printed = vsnprintf(buf, bufLen, message, l);
	int success = printed == messageLen - 1;
	if(success)
		g_log(buf);

	free(buf);

	// Buffer too small? Second try
	if(!success)
	{
		bufLen = messageLen * 8;
		buf = (char*) malloc(bufLen);
		if(!buf)
		{
			g_log("failed to allocate buffer for kernel logging on retry");
			return;
		}

		vsnprintf(buf, bufLen, message, lc);
		g_log(buf);

		free(buf);
	}

	va_end(lc);
}
