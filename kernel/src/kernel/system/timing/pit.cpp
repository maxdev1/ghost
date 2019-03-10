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

#include "kernel/system/timing/pit.hpp"
#include "shared/system/io_port.hpp"
#include "shared/logger/logger.hpp"

static uint32_t timerClocking;
static uint32_t sleepDivisor;

void pitPrepareSleep(uint32_t microseconds)
{
	// Sanity check
	if(microseconds > 54 * 1000)
	{
		logWarn("%! illegal use of sleep. may only sleep up to 54000 microseconds", "pit");
		microseconds = 0;
	}

	// Disable the speaker
	uint8_t speakerControlByte = ioPortReadByte(0x61);
	speakerControlByte &= ~2; // set bit 1 to 0 (disables speaker output)
	ioPortWriteByte(0x61, speakerControlByte);

	// Initialize PIT
	ioPortWriteByte(0x43, PIT_CHANNEL_2 | PIT_OPMODE_0_IOTC | PIT_ACCESS_LOHIBYTE);

	// Configure PIT, calculate divisor for the requested microseconds
	sleepDivisor = PIT_FREQUENCY / (1000000 / microseconds);
}

void pitPerformSleep()
{

	// Write the prepared sleep divisor
	ioPortWriteByte(0x42, sleepDivisor & 0xFF);
	ioPortWriteByte(0x42, sleepDivisor >> 8);

	// Reset the PIT counter and let it start
	uint8_t pitControlByte = ioPortReadByte(0x61);
	ioPortWriteByte(0x61, (uint8_t) pitControlByte & ~1);	// clear bit 0
	ioPortWriteByte(0x61, (uint8_t) pitControlByte | 1);		// set bit 0

	// Wait for PIT counter to reach 0
	while(!(ioPortReadByte(0x61) & 0x20))
	{
	}
}

void pitStartAsTimer(uint32_t hz)
{
	logDebug("%! started as timer on %i hertz", "pit", hz);

	timerClocking = hz;
	uint32_t divisor = PIT_FREQUENCY / hz; // Calculate the divisor
	ioPortWriteByte(0x43, PIT_CHANNEL_0 | PIT_ACCESS_LOHIBYTE | PIT_OPMODE_3_SQUARE_WAV);
	ioPortWriteByte(0x40, divisor & 0xFF); // Set low byte of the divisor
	ioPortWriteByte(0x40, divisor >> 8); // Set high byte of the divisor
}

uint32_t pitGetTimerClocking()
{
	return timerClocking;
}

