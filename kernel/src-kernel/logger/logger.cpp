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
#include <system/smp/global_recursive_lock.hpp>
#include <utils/string.hpp>
#include <video/console_video.hpp>

#include <system/system.hpp>
#include <tasking/tasking.hpp>

static g_global_recursive_lock printLock;

/**
 *
 */
void g_logger::print(const char* message, ...) {
	printLock.lock();
	va_list valist;
	va_start(valist, message);
	g_logger::printFormatted(message, valist);
	va_end(valist);
	printLock.unlock();
}

/**
 *
 */
void g_logger::println(const char* message, ...) {
	printLock.lock();
	va_list valist;
	va_start(valist, message);
	g_logger::printFormatted(message, valist);
	va_end(valist);
	g_logger::printCharacter('\n');
	printLock.unlock();
}

/**
 *
 */
void g_logger::manualLock() {
	printLock.lock();
}

/**
 *
 */
void g_logger::manualUnlock() {
	printLock.unlock();
}

