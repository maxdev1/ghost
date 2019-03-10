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

#include "kernel/system/smp.hpp"
#include "shared/system/mutex.hpp"
#include "shared/logger/logger.hpp"
#include "shared/system/mutex.hpp"

#include "shared/utils/string.hpp"
#include "shared/video/console_video.hpp"

static g_mutex printLock;

void loggerInitialize()
{
	mutexInitialize(&printLock);
}

void loggerPrintLocked(const char* message, ...)
{
	mutexAcquire(&printLock);

	va_list valist;
	va_start(valist, message);
	loggerPrintFormatted(message, valist);
	va_end(valist);

	mutexRelease(&printLock);
}

void loggerPrintlnLocked(const char* message, ...)
{
	mutexAcquire(&printLock);

	va_list valist;
	va_start(valist, message);
	loggerPrintFormatted(message, valist);
	va_end(valist);
	loggerPrintCharacter('\n');

	mutexRelease(&printLock);
}

void loggerPrintUnlocked(const char* message, ...)
{
	va_list valist;
	va_start(valist, message);
	loggerPrintFormatted(message, valist);
	va_end(valist);
}

void loggerPrintlnUnlocked(const char* message, ...)
{
	va_list valist;
	va_start(valist, message);
	loggerPrintFormatted(message, valist);
	va_end(valist);
	loggerPrintCharacter('\n');
}

void loggerManualLock() {
	mutexAcquire(&printLock);
}

void loggerManualUnlock() {
	mutexRelease(&printLock);
}
