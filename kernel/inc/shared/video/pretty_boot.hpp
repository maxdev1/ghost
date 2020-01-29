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

#ifndef __PRETTY_BOOT__
#define __PRETTY_BOOT__

#include <stdarg.h>
#include "ghost/stdint.h"
#include "build_config.hpp"


#define G_PRETTY_BOOT_PROGRESS_BAR_Y_POS	15
#define G_PRETTY_BOOT_LOGO_Y_POS			7


#if G_PRETTY_BOOT
#define G_PRETTY_BOOT_STATUS(text, percent) prettyBootUpdateStatus(text, percent)
#define G_PRETTY_BOOT_FAIL(text) prettyBootFail(text)
#else
#define G_PRETTY_BOOT_STATUS(text, percent)
#define G_PRETTY_BOOT_FAIL(text)
#endif

void prettyBootEnable(bool clear_screen = true);

void prettyBootPrintProgressBar(int percent, uint8_t color);

void prettyBootPrintCentered(const char* string, int y, uint8_t color);

void prettyBootUpdateStatus(const char* string, int percent);

void prettyBootFail(const char* string);

#endif
