/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef CORE_ARGUMENTS
#define CORE_ARGUMENTS

#include <stdint.h>
#include <string.h>

static inline bool coreArgumentIsHelp(const char* text)
{
	return text && (!strcmp(text, "-h") || !strcmp(text, "--help"));
}

static inline bool coreIsDecimalNumber(const char* text)
{
	if(!text || !*text)
		return false;

	for(auto digit = text; *digit; ++digit)
		if(*digit < '0' || *digit > '9')
			return false;

	return true;
}

/**
 * Parses a decimal prefix without consuming its suffix. Failed parses leave
 * the cursor and result unchanged, including on overflow of the chosen limit.
 */
static inline bool coreParseUnsignedPrefix(const char*& text, uint64_t& result, uint64_t maximum = UINT64_MAX)
{
	if(!text || *text < '0' || *text > '9')
		return false;

	auto cursor = text;
	uint64_t value = 0;
	while(*cursor >= '0' && *cursor <= '9')
	{
		auto digit = unsigned(*cursor - '0');
		if(value > maximum / 10 || (value == maximum / 10 && digit > maximum % 10))
			return false;

		value = value * 10 + digit;
		++cursor;
	}

	text = cursor;
	result = value;
	return true;
}

static inline bool coreParseUnsigned(const char* text, uint64_t* result, uint64_t maximum = UINT64_MAX)
{
	uint64_t value;
	if(!result || !coreParseUnsignedPrefix(text, value, maximum) || *text)
		return false;

	*result = value;
	return true;
}

#endif
