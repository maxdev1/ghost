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

#include <video/pretty_boot.hpp>
#include <video/console_video.hpp>
#include <utils/string.hpp>
#include <memory/memory.hpp>
#include <logger/logger.hpp>

const static int progressbar_ypos = 15;
const static int logo_ypos = 7;

/**
 *
 */
void g_pretty_boot::enable(bool clear_screen) {
	g_logger::setVideo(false);
	if (clear_screen) {
		g_console_video::clear();
	}

	g_console_video::putString(15, logo_ypos, "  \xDC\xDC\xDC\xDC  \xDC          \xDC    \xDC  \xDC                \xDC  ", 0x09);
	g_console_video::putString(15, logo_ypos + 1, " \xDE\xDB  \xDB\xDD\xDE\xDB         \xDE\xDB   \xDE\xDB \xDE\xDD               \xDE\xDB  ", 0x09);
	g_console_video::putString(15, logo_ypos + 2,
			" \xDE\xDB \xDC\xDC \xDE\xDB\xDC\xDC \xDC\xDC\xDC \xDC\xDC\xDE\xDB\xDC  \xDE\xDB\xDB\xDB  \xDC\xDC\xDC \xDC\xDC \xDC\xDC\xDC \xDC\xDC\xDC\xDE\xDB  ",
			0x09);
	g_console_video::putString(15, logo_ypos + 3,
			" \xDE\xDB  \xDB\xDD\xDE\xDB \xDB\xDE\xDB \xDB\xDD\xDF\xDC\xDE\xDB   \xDE\xDB \xDE\xDD\xDE\xDB \xDF\xDE\xDB \xDE\xDB \xDB\xDE\xDB \xDF\xDE\xDB  ",
			0x09);
	g_console_video::putString(15, logo_ypos + 4,
			"  \xDF\xDF\xDF\xDF  \xDF \xDF \xDF\xDF\xDF \xDF\xDF \xDF    \xDF  \xDF \xDF\xDF\xDF \xDF  \xDF \xDF \xDF\xDF\xDF \xDF\xDF ", 0x09);

	g_console_video::putString(0, 0, "Version 0.5.6", 0x07);
	print_centered("Copyright (c) 2012-2016 Max Schl\x81ssel", 23, 0x07);

	g_console_video::setVisualCursor(-1, -1);
}

/**
 *
 */
void g_pretty_boot::print_progress_bar(int percent, uint8_t color) {

	for (int i = 30; i < 50; i++) {
		g_console_video::putChar(i, progressbar_ypos, ' ', 0x80);
	}

	double cells = (20.0 / 100.0) * percent;
	if (cells < 1) {
		cells = 1;
	}

	for (int i = 30; i < 30 + (int) cells; i++) {
		g_console_video::putChar(i, progressbar_ypos, ' ', color);
	}
}

/**
 *
 */
void g_pretty_boot::print_centered(const char* string, int y, uint8_t color) {

	int strl = g_string::length(string);
	int left_bound = 40 - strl / 2;

	for (int i = 0; i < 80; i++) {
		g_console_video::putChar(i, y, ' ', 0x00);
	}
	g_console_video::putString(left_bound, y, string, color);
}

/**
 *
 */
void g_pretty_boot::update_status(const char* string, int percent) {
	print_progress_bar(percent, 0x90);
	print_centered(string, progressbar_ypos + 2, 0x0F);
}

/**
 *
 */
void g_pretty_boot::fail(const char* string) {
	print_progress_bar(100, 0xC0);
	print_centered(string, progressbar_ypos + 2, 0x0F);
}
