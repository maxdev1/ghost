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

#ifndef __LOGGER__
#define __LOGGER__

#include "stdarg.h"
#include "ghost/stdint.h"

#include "shared/logger/logger_macros.hpp"

void loggerInitialize();

void loggerPrintLocked(const char *message, ...);

void loggerPrintlnLocked(const char *message, ...);

void loggerPrintUnlocked(const char *message, ...);

void loggerPrintlnUnlocked(const char *message, ...);

void loggerManualLock();

void loggerManualUnlock();

void loggerEnableSerial(bool enable);

void loggerEnableVideo(bool video);

void loggerPrintPlain(const char *message);

void loggerPrintCharacter(char c);

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
void loggerPrintFormatted(const char *message, va_list va);

/**
 * Prints a number with a base.
 *
 * @param number	the number to print
 * @param base		the base to use
 * 					- if the base is 10 then signs are added
 * 					- if the base is 16 a preceding '0x' is added
 */
void loggerPrintNumber(uint32_t number, uint16_t base);

#endif
