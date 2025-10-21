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

#include "kernel/video/pretty_boot.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/utils/string.hpp"
#include "kernel/video/console_video.hpp"

void prettyBootEnable(bool clearScreen)
{
    loggerEnableVideo(false);
    if(clearScreen)
        consoleVideoClear();

    consoleVideoPutString(G_PRETTY_BOOT_LOGO_X_POS, G_PRETTY_BOOT_LOGO_Y_POS,
                          "  \xDC\xDC\xDC\xDC  \xDC          \xDC  ",
                          0x09);
    consoleVideoPutString(G_PRETTY_BOOT_LOGO_X_POS, G_PRETTY_BOOT_LOGO_Y_POS + 1,
                          " \xDE\xDB  \xDB\xDD\xDE\xDB         \xDE\xDB  ",
                          0x09);
    consoleVideoPutString(G_PRETTY_BOOT_LOGO_X_POS, G_PRETTY_BOOT_LOGO_Y_POS + 2,
                          " \xDE\xDB \xDC\xDC \xDE\xDB\xDC\xDC \xDC\xDC\xDC \xDC\xDC\xDE\xDB\xDC ",
                          0x09);
    consoleVideoPutString(G_PRETTY_BOOT_LOGO_X_POS, G_PRETTY_BOOT_LOGO_Y_POS + 3,
                          " \xDE\xDB  \xDB\xDD\xDE\xDB \xDB\xDE\xDB \xDB\xDD\xDF\xDC\xDE\xDB  ",
                          0x09);
    consoleVideoPutString(G_PRETTY_BOOT_LOGO_X_POS, G_PRETTY_BOOT_LOGO_Y_POS + 4,
                          "  \xDF\xDF\xDF\xDF  \xDF \xDF \xDF\xDF\xDF \xDF\xDF \xDF  ",
                          0x09);

    prettyBootPrintCentered("Version " STR(G_VERSION_MAJOR) "." STR(G_VERSION_MINOR) "." STR(G_VERSION_PATCH), 17, 0x07);
    prettyBootPrintCentered("(c) 2015-2025 Max Schl\x81ssel", 23, 0x07);

    consoleVideoSetVisualCursor(-1, -1);
}

void prettyBootPrintProgressBar(int percent, uint8_t color)
{
    double cells = (20.0 / 100.0) * percent;
    if(cells < 1)
        cells = 1;

    for(int i = 30; i < 30 + (int) cells; i++)
        consoleVideoPutChar(i, G_PRETTY_BOOT_PROGRESS_BAR_Y_POS, '\xDC', color);
    for(int i = 30 + cells; i < 50; i++)
        consoleVideoPutChar(i, G_PRETTY_BOOT_PROGRESS_BAR_Y_POS, '\xDC', 0x07);
}

void prettyBootPrintCentered(const char* string, int y, uint8_t color)
{
    int strl = stringLength(string);
    int left_bound = 40 - strl / 2;

    for(int i = 0; i < 80; i++)
        consoleVideoPutChar(i, y, ' ', 0x00);
    consoleVideoPutString(left_bound, y, string, color);
}

void prettyBootUpdateStatus(const char* string, int percent)
{
    prettyBootPrintProgressBar(percent, 0x09);
    if(string)
        prettyBootPrintCentered(string, G_PRETTY_BOOT_PROGRESS_BAR_Y_POS + 2, 0x0F);
}

void prettyBootFail(const char* string)
{
    prettyBootPrintProgressBar(100, 0xC0);
    prettyBootPrintCentered(string, G_PRETTY_BOOT_PROGRESS_BAR_Y_POS + 2, 0x0F);
}
