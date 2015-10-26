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

#include "debug/debug_interface.hpp"
#include "system/serial/serial_port.hpp"

bool g_debug_interface_initialized = false;
static uint16_t serialPort;

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL
static const int logBufferLength = 512;
static char logBuffer[logBufferLength];
static int logBuffered = 0;
#endif

/**
 *
 */
void g_debug_interface::initialize(uint16_t port) {
	g_debug_interface_initialized = true;
	serialPort = port;
}

/**
 *
 */
void g_debug_interface::writeLogCharacter(char c) {

	if (!g_debug_interface_initialized) {
		return;
	}

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_PLAIN_LOG
	g_serial_port::write(serialPort, c);

#elif G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL

	// put char in buffer
	logBuffer[logBuffered++] = c;

	// write buffer to output on newline or full buffer
	if (c == '\n' || logBuffered == logBufferLength - 1) {

		writeShort(G_DEBUG_MESSAGE_LOG);

		for (int i = 0; i < logBuffered; i++) {
			g_serial_port::write(serialPort, logBuffer[i]);
		}
		g_serial_port::write(serialPort, 0);

		logBuffered = 0;
	}
#endif
}

/**
 *
 */
void g_debug_interface::writeByte(uint8_t value) {

	g_serial_port::write(serialPort, value);
}

/**
 *
 */
void g_debug_interface::writeShort(uint16_t value) {

	g_serial_port::write(serialPort, (value >> 0) & 0xFF);
	g_serial_port::write(serialPort, (value >> 8) & 0xFF);
}

/**
 *
 */
void g_debug_interface::writeInt(uint32_t value) {

	g_serial_port::write(serialPort, (value >> 0) & 0xFF);
	g_serial_port::write(serialPort, (value >> 8) & 0xFF);
	g_serial_port::write(serialPort, (value >> 16) & 0xFF);
	g_serial_port::write(serialPort, (value >> 24) & 0xFF);
}

/**
 *
 */
void g_debug_interface::writeLong(uint64_t value) {

	g_serial_port::write(serialPort, (value >> 0) & 0xFF);
	g_serial_port::write(serialPort, (value >> 8) & 0xFF);
	g_serial_port::write(serialPort, (value >> 16) & 0xFF);
	g_serial_port::write(serialPort, (value >> 24) & 0xFF);
	g_serial_port::write(serialPort, (value >> 32) & 0xFF);
	g_serial_port::write(serialPort, (value >> 40) & 0xFF);
	g_serial_port::write(serialPort, (value >> 48) & 0xFF);
	g_serial_port::write(serialPort, (value >> 56) & 0xFF);
}

/**
 *
 */
void g_debug_interface::writeString(const char* string) {

	const char* p = string;
	while (*p) {
		g_serial_port::write(serialPort, *p++);
	}
	g_serial_port::write(serialPort, 0);
}
