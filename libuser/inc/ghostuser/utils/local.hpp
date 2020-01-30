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

#ifdef __cplusplus

#ifndef GHOST_SHARED_UTILS_LOCAL
#define GHOST_SHARED_UTILS_LOCAL

#include "ghost/common.h"

/**
 *
 */
template<typename T>
class g_local {
public:

	/**
	 *
	 */
	g_local(T* value) :
			value(value) {
	}

	/**
	 *
	 */
	g_local() :
			value(0) {
	}

	/**
	 *
	 */
	~g_local() {
		if (value) {
			delete value;
		}
	}

	/**
	 *
	 */
	g_local(const g_local& rhs) = delete;

	/**
	 *
	 */
	g_local& operator=(const g_local& rhs) = delete;

	/**
	 *
	 */
	T* operator()() const {
		return value;
	}

	/**
	 *
	 */
	T* release() {

		T* released_value = value;
		value = 0;
		return released_value;
	}

private:

	T* value = 0;
};

#endif

#endif
