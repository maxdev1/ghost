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

#include <string>

#ifndef GHOSTLIBRARY_UTILS_UTILS
#define GHOSTLIBRARY_UTILS_UTILS

/**
 *
 */
class g_utils {
public:
	static std::string trim(std::string str);

	static uint8_t inportByte(uint16_t port);
	static void outportByte(uint16_t port, uint8_t data);
	static uint16_t inportShort(uint16_t port);
	static void outportShort(uint16_t port, uint16_t data);
	static uint32_t inportInt(uint16_t port);
	static void outportInt(uint16_t port, uint32_t data);
};

#endif
