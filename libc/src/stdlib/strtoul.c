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

#include <ctype.h>
#include <limits.h>

unsigned long strtoul(const char* str, char** endptr, int base)
{
	const char* p = str;
	while(isspace(*p))
		p++;

	int neg = 0;
	if(*p == '+' || *p == '-')
	{
		if(*p == '-')
			neg = 1;
		p++;
	}

	if(base == 0)
	{
		if(*p == '0')
		{
			if(p[1] == 'x' || p[1] == 'X')
			{
				base = 16;
				p += 2;
			}
			else
			{
				base = 8;
				p++;
			}
		}
		else
			base = 10;
	}
	else if(base == 16 && *p == '0' && (p[1] == 'x' || p[1] == 'X'))
		p += 2;

	unsigned long result = 0;
	while(*p)
	{
		int digit;
		if(isdigit(*p))
			digit = *p - '0';
		else if(isalpha(*p))
			digit = (tolower(*p) - 'a') + 10;
		else
			break;
		if(digit >= base)
			break;

		if(result > (ULONG_MAX - digit) / base)
		{
			result = ULONG_MAX;
			break;
		}

		result = result * base + digit;
		p++;
	}

	if(endptr)
		*endptr = (char*) p;
	return neg ? (unsigned long) (-(long) result) : result;
}
