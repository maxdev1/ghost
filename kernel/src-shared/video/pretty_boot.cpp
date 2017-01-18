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

/**
 *
 */
void g_pretty_boot::enable(bool clear_screen) {
	g_logger::setVideo(false);
	if (clear_screen) {
		g_console_video::clear();
	}
	print_centered("Ghost", 10, 0x09);
	g_console_video::setVisualCursor(-1, -1);
}

/**
 *
 */
void g_pretty_boot::print_progress_bar(int percent, uint8_t color) {

	for (int i = 30; i < 50; i++) {
		g_console_video::putChar(i, 12, ' ', 0x80);
	}

	double cells = (20.0 / 100.0) * percent;
	if (cells < 1) {
		cells = 1;
	}

	for (int i = 30; i < 30 + (int) cells; i++) {
		g_console_video::putChar(i, 12, ' ', color);
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
	print_centered(string, 14, 0x07);
}

/**
 *
 */
void g_pretty_boot::fail(const char* string) {
	print_progress_bar(100, 0xC0);
	print_centered(string, 14, 0x0F);
}
