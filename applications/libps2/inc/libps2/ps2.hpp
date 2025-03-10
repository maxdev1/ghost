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

#ifndef __LIBPS2__
#define __LIBPS2__

#include <stdint.h>

typedef uint8_t ps2_status_t;
#define G_PS2_STATUS_SUCCESS                ((ps2_status_t) 0)
#define G_PS2_STATUS_FAILED_INITIALIZE      ((ps2_status_t) 1)
#define G_PS2_STATUS_FAILED_ACKNOWLEDGE     ((ps2_status_t) 2)


#define G_PS2_DATA_PORT     0x60
#define G_PS2_STATUS_PORT   0x64

/**
 * Initializes the PS2 handling.
 */
ps2_status_t ps2Initialize(void(*mouseCallback)(int16_t, int16_t, uint8_t, int8_t), void(*keyboardCallback)(uint8_t));

#endif
