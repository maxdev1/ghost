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

#ifndef __GHOST_DEBUG_INTERFACE__
#define __GHOST_DEBUG_INTERFACE__

#include "ghost/stdint.h"
#include "build_config.hpp"

#include "shared/debug/debug_protocol.hpp"
#include "shared/debug/debug_interface_mode.hpp"

extern bool debugInterfaceInitialized;

void debugInterfaceInitialize(uint16_t port);

void debugInterfaceWriteLogCharacter(char c);

void debugInterfaceWriteByte(uint8_t value);

void debugInterfaceWriteShort(uint16_t value);

void debugInterfaceWriteInt(uint32_t value);

void debugInterfaceWriteLong(uint64_t value);

void debugInterfaceWriteString(const char *string);

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL
void debugInterfaceFullWriteLogCharacter(char c);
#endif

#endif
