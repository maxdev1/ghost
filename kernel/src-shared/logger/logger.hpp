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

#ifndef GHOST_SHARED_LOGGER_LOGGER
#define GHOST_SHARED_LOGGER_LOGGER

#include <stdarg.h>
#include "ghost/stdint.h"
#include <logger/log_macros.hpp>

/**
 * g_logger class
 */
class g_logger {
public:

	/**
	 *
	 */
	static void manualLock();

	/**
	 *
	 */
	static void manualUnlock();

	/**
	 *
	 */
	static void enableSerialPortLogging();

	/**
	 * Enables or disables the serial output.
	 *
	 * @param serial	whether to enable the serial output
	 */
	static void setSerial(bool serial);

	/**
	 * Enables or disables the video output.
	 *
	 * @param video		whether to enable the video output
	 */
	static void setVideo(bool video);

	/**
	 * Prints the given format message using the given parameters.
	 *
	 * @see printFormatted
	 *
	 * @param message		the message pattern
	 * @param ...			the parameters
	 */
	static void print(const char* message, ...);

	/**
	 * Prints a line using the print method, appending a line break.
	 *
	 * @see printFormatted
	 *
	 * @param message		the message pattern
	 * @param ...			the parameters
	 */
	static void println(const char* message, ...);

	/**
	 * Prints the given message, using the arguments from the given variable
	 * argument list, formatting the values as:
	 *
	 * %i		as a value with base 10 using printNumber
	 * %h		as a value with base 16 using printNumber
	 * %b		true or false
	 * %s		a plain string
	 * %%		a '%' character
	 *
	 * The following options are also available and should be added only at
	 * the start of the pattern:
	 *
	 * %!		adds the respective value as an header to the line
	 * %*		allows changing the color of the header to the respective value
	 * %#		indents the following message by the width of an header
	 * 			(this option does not require a value!)
	 *
	 * @param message	the message pattern
	 * @param va		the list of arguments
	 */
	static void printFormatted(const char* message, va_list va);

	/**
	 * Prints a number with a base.
	 *
	 * @param number	the number to print
	 * @param base		the base to use
	 * 					- if the base is 10 then signs are added
	 * 					- if the base is 16 a preceding '0x' is added
	 */
	static void printNumber(uint32_t number, uint16_t base);

	/**
	 * Prints the given message.
	 *
	 * @param message	the message to print
	 */
	static void printPlain(const char* message);

	/**
	 * Prints the given character, redirecting it the video and/or serial output.
	 *
	 * @param c			the character to print
	 */
	static void printCharacter(char c);

};

#endif
