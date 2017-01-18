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

#ifndef GHOST_SHARED_VIDEO_PRETTYBOOT
#define GHOST_SHARED_VIDEO_PRETTYBOOT

#include <stdarg.h>
#include "ghost/stdint.h"
#include <build_config.hpp>

#if G_PRETTY_BOOT
#define G_PRETTY_BOOT_STATUS(text, percent)		g_pretty_boot::update_status(text, percent)
#define G_PRETTY_BOOT_FAIL(text)				g_pretty_boot::fail(text)
#else
#define G_PRETTY_BOOT_STATUS(text, percent)
#define G_PRETTY_BOOT_FAIL(text)
#endif

/**
 *
 */
class g_pretty_boot {
public:
	static void enable(bool clear_screen = true);

	static void print_progress_bar(int percent, uint8_t color);
	static void print_centered(const char* string, int y, uint8_t color);

	static void update_status(const char* string, int percent);
	static void fail(const char* string);
};

#endif
