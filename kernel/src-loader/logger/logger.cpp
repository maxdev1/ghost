#/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

/**
 *
 */
void g_logger::print(const char* message, ...) {
	va_list valist;
	va_start(valist, message);
	g_logger::printFormatted(message, valist);
	va_end(valist);
}

/**
 *
 */
void g_logger::println(const char* message, ...) {
	va_list valist;
	va_start(valist, message);
	g_logger::printFormatted(message, valist);
	va_end(valist);
	g_logger::printCharacter('\n');
}

/**
 * Only needed in kernel implementation
 */
void g_logger::manualLock() {
}

/**
 * Only needed in kernel implementation
 */
void g_logger::manualUnlock() {
}

