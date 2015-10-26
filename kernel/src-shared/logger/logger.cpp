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

#include <logger/logger.hpp>
#include <utils/string.hpp>
#include <video/console_video.hpp>
#include <system/serial/serial_port.hpp>
#include <system/bios_data_area.hpp>
#include "debug/debug_interface.hpp"

/**
 * Width of a log entry header
 */
static const uint32_t HEADER_WIDTH = 15;

/**
 * g_logger variables
 */
static bool logSerial = false;
static bool logVideo = true;

/**
 * 
 */
void g_logger::enableSerialPortLogging() {
	logSerial = true;
}

/**
 * 
 */
void g_logger::setSerial(bool serial) {
	logSerial = serial;
}

/**
 * 
 */
void g_logger::setVideo(bool video) {
	logVideo = video;
}

/**
 * 
 */
void g_logger::printFormatted(const char* message_const, va_list valist) {
	char* message = (char*) message_const;

	uint16_t headerColor = 0x07;

	while (*message) {
		if (*message == '%') {
			++message;

			if (*message == 'i') { // integer
				int32_t val = va_arg(valist, int32_t);
				printNumber(val, 10);

			} else if (*message == 'h') { // positive hex number
				uint32_t val = va_arg(valist, uint32_t);
				printPlain("0x");
				printNumber(val, 16);

			} else if (*message == 'b') { // boolean
				int val = va_arg(valist, int);
				printPlain((const char*) (val ? "true" : "false"));

			} else if (*message == 'c') { // char
				int val = va_arg(valist, int);
				printCharacter((char) val);

			} else if (*message == 's') { // string
				char* val = va_arg(valist, char*);
				printPlain(val);

			} else if (*message == '#') { // indented printing
				for (uint32_t i = 0; i < HEADER_WIDTH + 2; i++) {
					printCharacter(' ');
				}

			} else if (*message == '!') { // indented header printing
				char* val = va_arg(valist, char*);
				uint32_t headerlen = g_string::length(val);

				if (headerlen < HEADER_WIDTH) {
					for (uint32_t i = 0; i < HEADER_WIDTH - headerlen; i++) {
						printCharacter(' ');
					}
				}

				printCharacter('[');
				g_console_video::setColor(headerColor);
				print(val);
				g_console_video::setColor(0x0F);
				printCharacter(']');

			} else if (*message == '%') { // escaped %
				printCharacter(*message);

			} else if (*message == '*') { // header color change
				headerColor = (uint16_t) (va_arg(valist, int));
			}
		} else {
			printCharacter(*message);
		}
		++message;
	}
}

/**
 * 
 */
void g_logger::printNumber(uint32_t number, uint16_t base) {

	// Remember if negative
	uint8_t negative = 0;
	if (base == 10) {
		negative = ((int32_t) number) < 0;

		if (negative) {
			number = -number;
		}
	}

	// Write chars in reverse order, not nullterminated
	char revbuf[32];

	char* cbufp = revbuf;
	int len = 0;
	do {
		*cbufp++ = "0123456789ABCDEF"[number % base];
		++len;
		number /= base;
	} while (number);

	// If base is 16, write 0's until 8
	if (base == 16) {
		while (len < 8) {
			*cbufp++ = '0';
			++len;
		}
	}

	// Reverse buffer
	char buf[len + 1];
	for (int i = 0; i < len; i++) {
		buf[i] = revbuf[len - i - 1];
	}
	buf[len] = 0;

	// Print number
	if (negative) {
		printCharacter('-');
	}
	printPlain(buf);
}

/**
 * 
 */
void g_logger::printPlain(const char* message_const) {
	char* message = (char*) message_const;
	while (*message) {
		printCharacter(*message++);
	}
}

/**
 * 
 */
void g_logger::printCharacter(char c) {
	if (logVideo) {
		g_console_video::print(c);
	}

	if (logSerial) {
		g_debug_interface::writeLogCharacter(c);
	}
}
