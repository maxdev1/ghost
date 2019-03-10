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

#include "shared/video/pretty_boot.hpp"
#include "shared/video/console_video.hpp"
#include "shared/utils/string.hpp"
#include "shared/memory/memory.hpp"
#include "shared/logger/logger.hpp"

void prettyBootEnable(bool clearScreen)
{
	loggerEnableVideo(false);
	if(clearScreen)
		consoleVideoClear();

	consoleVideoPutString(15, G_PRETTY_BOOT_LOGO_Y_POS, "  \xDC\xDC\xDC\xDC  \xDC          \xDC    \xDC  \xDC                \xDC  ", 0x09);
	consoleVideoPutString(15, G_PRETTY_BOOT_LOGO_Y_POS + 1, " \xDE\xDB  \xDB\xDD\xDE\xDB         \xDE\xDB   \xDE\xDB \xDE\xDD               \xDE\xDB  ", 0x09);
	consoleVideoPutString(15, G_PRETTY_BOOT_LOGO_Y_POS + 2,
			" \xDE\xDB \xDC\xDC \xDE\xDB\xDC\xDC \xDC\xDC\xDC \xDC\xDC\xDE\xDB\xDC  \xDE\xDB\xDB\xDB  \xDC\xDC\xDC \xDC\xDC \xDC\xDC\xDC \xDC\xDC\xDC\xDE\xDB  ",
			0x09);
	consoleVideoPutString(15, G_PRETTY_BOOT_LOGO_Y_POS + 3,
			" \xDE\xDB  \xDB\xDD\xDE\xDB \xDB\xDE\xDB \xDB\xDD\xDF\xDC\xDE\xDB   \xDE\xDB \xDE\xDD\xDE\xDB \xDF\xDE\xDB \xDE\xDB \xDB\xDE\xDB \xDF\xDE\xDB  ",
			0x09);
	consoleVideoPutString(15, G_PRETTY_BOOT_LOGO_Y_POS + 4,
			"  \xDF\xDF\xDF\xDF  \xDF \xDF \xDF\xDF\xDF \xDF\xDF \xDF    \xDF  \xDF \xDF\xDF\xDF \xDF  \xDF \xDF \xDF\xDF\xDF \xDF\xDF ", 0x09);

	consoleVideoPutString(0, 0, "Version 0.5.6", 0x07);
	prettyBootPrintCentered("Copyright (c) 2012-2016 Max Schl\x81ssel", 23, 0x07);

	consoleVideoSetVisualCursor(-1, -1);
}

void prettyBootPrintProgressBar(int percent, uint8_t color)
{

	for(int i = 30; i < 50; i++)
		consoleVideoPutChar(i, G_PRETTY_BOOT_PROGRESS_BAR_Y_POS, ' ', 0x80);

	double cells = (20.0 / 100.0) * percent;
	if(cells < 1)
		cells = 1;

	for(int i = 30; i < 30 + (int) cells; i++)
		consoleVideoPutChar(i, G_PRETTY_BOOT_PROGRESS_BAR_Y_POS, ' ', color);
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
	prettyBootPrintProgressBar(percent, 0x90);
	prettyBootPrintCentered(string, G_PRETTY_BOOT_PROGRESS_BAR_Y_POS + 2, 0x0F);
}

void prettyBootFail(const char* string)
{
	prettyBootPrintProgressBar(100, 0xC0);
	prettyBootPrintCentered(string, G_PRETTY_BOOT_PROGRESS_BAR_Y_POS + 2, 0x0F);
}
