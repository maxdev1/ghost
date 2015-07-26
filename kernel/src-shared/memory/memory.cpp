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

#include <memory/memory.hpp>

/**
 * 
 */
void* g_memory::setBytes(void* target, uint8_t value, int32_t length) {

	uint8_t* pos = (uint8_t*) target;
	while (length--) {
		*pos++ = (uint8_t) value;
	}
	return target;
}

/**
 * 
 */
void* g_memory::setWords(void* target, uint16_t value, int32_t length) {

	uint16_t* pos = (uint16_t*) target;
	while (length--) {
		*pos++ = (uint16_t) value;
	}
	return target;
}

/**
 * 
 */
void* g_memory::copy(void* target, const void* source, int32_t length) {

	uint8_t *targetPos = (uint8_t*) target;
	const uint8_t *sourcePos = (const uint8_t*) source;

	while (length--) {
		*targetPos++ = *sourcePos++;
	}

	return target;
}
