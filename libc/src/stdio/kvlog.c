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

	uint32_t messageLen = strlen(message);
	uint32_t bufLen = messageLen * 2;

	char* buf = NULL;
	int printed = -1;

	for(;;)
	{
		free(buf);
		buf = malloc(bufLen);

		if(!buf)
		{
			g_log("failed to allocate buffer for kernel logging");
			va_end(lc);
			return;
		}

		va_list ltmp;
		va_copy(ltmp, lc);
		printed = vsnprintf(buf, bufLen, message, ltmp);
		va_end(ltmp);

		if(printed >= 0 && (uint32_t) printed < bufLen)
		{
			g_log(buf);
			break;
		}

		bufLen *= 2;
	}

	free(buf);
	va_end(lc);
}
