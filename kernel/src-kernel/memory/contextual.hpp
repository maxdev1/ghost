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

#ifndef GHOST_SHARED_UTILS_ADDRESS_SPACE_BOUND
#define GHOST_SHARED_UTILS_ADDRESS_SPACE_BOUND

#include "memory/address_space.hpp"
#include "kernel.hpp"

/**
 * Safe wrapper for variables that are bound to a specific address space (context).
 * Is used when the kernel operates on multiple address spaces to make sure that
 * page directory switches are done properly and erroneous reads/writes don't occur.
 */
template<typename T>
class g_contextual {
public:
	/**
	 *
	 */
	g_contextual() :
			value(0), space(0) {
	}

	/**
	 *
	 */
	g_contextual(T value, g_page_directory space) :
			value(value), space(space) {
	}

	/**
	 *
	 */
	g_contextual(const g_contextual& rhs) {
		this->value = rhs.value;
		this->space = rhs.space;
	}

	/**
	 *
	 */
	g_contextual& operator=(const g_contextual& rhs) {
		this->value = rhs.value;
		this->space = rhs.space;
		return *this;
	}

	/**
	 *
	 */
	T operator()() const {
		if (space != 0) {
			g_page_directory current_space = g_address_space::get_current_space();
			if (current_space != space) {
				g_kernel::panic("%! tried to access a value from within another context", "contextual");
			}
		}
		return value;
	}

	/**
	 *
	 */
	void set(T value, g_page_directory space) {
		this->value = value;
		this->space = space;
	}

private:
	T value;
	g_page_directory space;

};

#endif
