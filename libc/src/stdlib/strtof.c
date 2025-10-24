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
#include <math.h>

/**
 *
 */
float strtof(const char* str, char** endptr)
{
	const char* p = str;
	while(isspace(*p))
		p++;

	int sign = 1;
	if(*p == '+' || *p == '-')
	{
		if(*p == '-')
			sign = -1;
		p++;
	}

	double val = 0.0;
	while(isdigit(*p))
	{
		val = val * 10.0 + (*p - '0');
		p++;
	}

	if(*p == '.')
	{
		p++;
		double frac = 1.0;
		while(isdigit(*p))
		{
			frac *= 0.1;
			val += (*p - '0') * frac;
			p++;
		}
	}

	int exp_sign = 1;
	int exp_val = 0;
	if(*p == 'e' || *p == 'E')
	{
		p++;
		if(*p == '+' || *p == '-')
		{
			if(*p == '-')
				exp_sign = -1;
			p++;
		}
		while(isdigit(*p))
		{
			exp_val = exp_val * 10 + (*p - '0');
			p++;
		}
	}

	if(endptr)
		*endptr = (char*) p;
	return (float) (sign * val * pow(10.0, exp_sign * exp_val));
}
