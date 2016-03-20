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

#ifndef __GHOST_SHARED_SYSTEM_IOPORTS__
#define __GHOST_SHARED_SYSTEM_IOPORTS__

#include "ghost/stdint.h"

/**
 *
 */
class g_io_ports {
public:
	static uint8_t readByte(uint16_t port);
	static void writeByte(uint16_t port, uint8_t data);
	static uint16_t readShort(uint16_t port);
	static void writeShort(uint16_t port, uint16_t data);
	static uint32_t readInt(uint16_t port);
	static void writeInt(uint16_t port, uint32_t data);
};

#endif
