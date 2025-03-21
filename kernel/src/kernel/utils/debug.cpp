/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "kernel/utils/debug.hpp"
#include "kernel/system/timing/pit.hpp"
#include "shared/logger/logger.hpp"

void debugHardSleep(uint64_t millis)
{
	for(int i = 0; i < millis; i++)
	{
		pitPrepareSleep(1000); // 1ms
		pitPerformSleep();
	}
}

void hexDumpRow(void* location, int bytes, bool center = false)
{
	auto centerStr = center ? "<-" : "";

	if(bytes == 1)
	{
		auto loc8 = (uint8_t*) location;
		logInfo("%# %x: %h %h %h %h %h %h %h %h %c%c%c%c%c%c%c%c %s", location,
		        loc8[0], loc8[1], loc8[2], loc8[3], loc8[4], loc8[5], loc8[6], loc8[7],
		        loc8[0], loc8[1], loc8[2], loc8[3], loc8[4], loc8[5], loc8[6], loc8[7],
		        centerStr);
	}
	else if(bytes == 8)
	{
		auto loc64 = (uint64_t*) location;
		logInfo("%# %x: %x %s", location, *loc64, centerStr);
	}
}

void hexDump(void* location, int minus, int plus, int bytes)
{
	for(int i = minus; i > 0; i--)
		hexDumpRow((uint8_t*) location - i * bytes, bytes);

	hexDumpRow(location, bytes, true);

	for(int i = 1; i < plus + 1; ++i)
		hexDumpRow((uint8_t*) location + i * bytes, bytes);
}

void hexDump8(void* location, int minus, int plus)
{
	hexDump(location, minus, plus, 1);
}

void hexDump64(void* location, int minus, int plus)
{
	hexDump(location, minus, plus, 8);
}
