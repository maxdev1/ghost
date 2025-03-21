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

#include "shared/memory/memory.hpp"

g_bitmap_page_allocator memoryPhysicalAllocator;

void* memorySetBytes(void* target, uint8_t value, int32_t length)
{
	auto pos = (uint8_t*) target;

	while(length--)
		*pos++ = value;

	return target;
}

void* memorySetWords(void* target, uint16_t value, int32_t length)
{
	auto pos = (uint16_t*) target;

	while(length--)
		*pos++ =  value;

	return target;
}

void* memoryCopy(void* target, const void* source, int32_t size)
{
	auto targetPtr = (uint8_t*) target;
	auto sourcePtr = (const uint8_t*) source;

	// TODO qword copying

	while(size >= 4)
	{
		*(uint32_t*) targetPtr = *(const uint32_t*) sourcePtr;
		targetPtr += 4;
		sourcePtr += 4;
		size -= 4;
	}

	while(size >= 2)
	{
		*(uint16_t*) targetPtr = *(const uint16_t*) sourcePtr;
		targetPtr += 2;
		sourcePtr += 2;
		size -= 2;
	}

	while(size--)
		*targetPtr++ = *sourcePtr++;

	return target;
}
