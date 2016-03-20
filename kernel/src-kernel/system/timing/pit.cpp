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

#include <system/timing/pit.hpp>
#include <system/io_ports.hpp>
#include <logger/logger.hpp>

static uint32_t timerClocking;

static uint32_t sleepDivisor;
/**
 *
 */
void g_pit::prepareSleep(uint32_t microseconds) {

	// Sanity check
	if (microseconds > 54 * 1000) {
		g_log_warn("%! illegal use of sleep. may only sleep up to 54000 microseconds", "pit");
	}

	// Disable the speaker
	uint8_t speakerControlByte = g_io_ports::readByte(0x61);
	speakerControlByte &= ~2; // set bit 1 to 0 (disables speaker output)
	g_io_ports::writeByte(0x61, speakerControlByte);

	// Initialize PIT
	g_io_ports::writeByte(0x43, PIT_CHANNEL_2 | PIT_OPMODE_0_IOTC | PIT_ACCESS_LOHIBYTE);

	// Configure PIT, calculate divisor for the requested microseconds
	sleepDivisor = PIT_FREQUENCY / (1000000 / microseconds);
}

/**
 *
 */
void g_pit::performSleep() {

	// Write the prepared sleep divisor
	g_io_ports::writeByte(0x42, sleepDivisor & 0xFF);
	g_io_ports::writeByte(0x42, sleepDivisor >> 8);

	// Reset the PIT counter and let it start
	uint8_t pitControlByte = g_io_ports::readByte(0x61);
	g_io_ports::writeByte(0x61, (uint8_t) pitControlByte & ~1);	// clear bit 0
	g_io_ports::writeByte(0x61, (uint8_t) pitControlByte | 1);		// set bit 0

	// Wait for PIT counter to reach 0
	while (!(g_io_ports::readByte(0x61) & 0x20)) {
	}
}

/**
 * Sets the timer clocking
 */
void g_pit::startAsTimer(uint32_t hz) {
	g_log_debug("%! started as timer on %i hertz", "pit", hz);

	timerClocking = hz;
	uint32_t divisor = PIT_FREQUENCY / hz; // Calculate the divisor
	g_io_ports::writeByte(0x43, PIT_CHANNEL_0 | PIT_ACCESS_LOHIBYTE | PIT_OPMODE_3_SQUARE_WAV);
	g_io_ports::writeByte(0x40, divisor & 0xFF); // Set low byte of the divisor
	g_io_ports::writeByte(0x40, divisor >> 8); // Set high byte of the divisor
}

/**
 * Returns the last hz set as clocking
 */
uint32_t g_pit::getTimerClocking() {
	return timerClocking;
}

