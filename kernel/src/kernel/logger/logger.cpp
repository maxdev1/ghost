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
#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/system/smp.hpp"
#include "shared/system/mutex.hpp"
#include "shared/utils/string.hpp"
#include "shared/video/console_video.hpp"

void loggerPrintLocked(const char* message, ...)
{
    int intr = interruptsAreEnabled();
    if(intr)
        interruptsDisable();

    va_list valist;
    va_start(valist, message);
    loggerPrintFormatted(message, valist);
    va_end(valist);

    if(intr)
        interruptsEnable();
}

void loggerPrintlnLocked(const char* message, ...)
{
    int intr = interruptsAreEnabled();
    if(intr)
        interruptsDisable();
        
    va_list valist;
    va_start(valist, message);
    loggerPrintFormatted(message, valist);
    va_end(valist);
    loggerPrintCharacter('\n');

    if(intr)
        interruptsEnable();
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
