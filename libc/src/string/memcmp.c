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

#include "string.h"
#include "stdint.h"
#include "ghost.h"

/**
 *
 */
int memcmp(const void* mem_a, const void* mem_b, size_t len) {

	__G_DEBUG_TRACE(memcmp);

	const uint8_t* mem_a8 = (const uint8_t*) mem_a;
	const uint8_t* mem_b8 = (const uint8_t*) mem_b;

	for (size_t i = 0; i < len; i++) {
		if (mem_a8[i] > mem_b8[i]) {
			return 1;
		}
		if (mem_a8[i] < mem_b8[i]) {
			return -1;
		}
	}

	return 0;
}
