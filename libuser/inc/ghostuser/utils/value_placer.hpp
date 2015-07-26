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

#ifndef __GHOST_USER_LIBRARY__VALUE_PLACER__
#define __GHOST_USER_LIBRARY__VALUE_PLACER__

#include <cstdint>
#include <string.h>

/**
 *
 */
class g_value_placer {
private:
	uint8_t* buf;
	uint32_t pos;

public:
	g_value_placer(uint8_t* buf) :
			buf(buf), pos(0) {
	}

	template<typename T>
	void put(T val) {
		*((T*) &buf[pos]) = val;
		pos += sizeof(T);
	}

	void put(uint8_t* in, uint32_t len) {
		memcpy(&buf[pos], in, len);
		pos += len;
	}

	template<typename T>
	T get() {
		T val = *((T*) &buf[pos]);
		pos += sizeof(T);
		return val;
	}

	void get(uint8_t* out, uint32_t len) {
		memcpy(out, &buf[pos], len);
		pos += len;
	}

};

#endif
