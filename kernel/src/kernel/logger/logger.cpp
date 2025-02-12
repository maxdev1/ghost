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

#include "shared/logger/logger.hpp"
#include "shared/system/spinlock.hpp"
#include "kernel/system/interrupts/interrupts.hpp"

g_spinlock loggerLock = 0;

void loggerPrintLocked(const char* message, ...)
{
	INTERRUPTS_PAUSE;
	G_SPINLOCK_ACQUIRE(loggerLock);

	va_list valist;
	va_start(valist, message);
	loggerPrintFormatted(message, valist);
	va_end(valist);

	G_SPINLOCK_RELEASE(loggerLock);
	INTERRUPTS_RESUME;
}

void loggerPrintlnLocked(const char* message, ...)
{
	INTERRUPTS_PAUSE;
	G_SPINLOCK_ACQUIRE(loggerLock);

	va_list valist;
	va_start(valist, message);
	loggerPrintFormatted(message, valist);
	va_end(valist);
	loggerPrintCharacter('\n');

	G_SPINLOCK_RELEASE(loggerLock);
	INTERRUPTS_RESUME;
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
