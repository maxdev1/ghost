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

#include "string.h"
#include "stdint.h"

/**
 *
 */
void* memcpy(void* dest, const void* src, size_t num)
{

	uint8_t* targetPtr = (uint8_t*) dest;
	const uint8_t* sourcePtr = (const uint8_t*) src;

	// TODO qword copying

	while(num >= 4)
	{
		*(uint32_t*) targetPtr = *(const uint32_t*) sourcePtr;
		targetPtr += 4;
		sourcePtr += 4;
		num -= 4;
	}

	while(num >= 2)
	{
		*(uint16_t*) targetPtr = *(const uint16_t*) sourcePtr;
		targetPtr += 2;
		sourcePtr += 2;
		num -= 2;
	}

	while(num--)
		*targetPtr++ = *sourcePtr++;

	return dest;
}
