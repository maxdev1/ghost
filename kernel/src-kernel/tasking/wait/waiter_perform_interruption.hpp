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

#ifndef GHOST_MULTITASKING_WAIT_MANAGER_JOIN
#define GHOST_MULTITASKING_WAIT_MANAGER_JOIN

#include <tasking/wait/waiter.hpp>
#include <logger/logger.hpp>
#include <tasking/tasking.hpp>

/**
 * This handler causes the task that it's appended to to store it's current state
 * and go somewhere else in the code.
 */
class g_waiter_perform_interruption: public g_waiter {
private:
	uintptr_t entry;
	uintptr_t callback;

public:

	/**
	 *
	 */
	g_waiter_perform_interruption(uintptr_t entry, uintptr_t callback) :
			entry(entry), callback(callback) {
	}

	/**
	 *
	 */
	virtual bool checkWaiting(g_thread* task);

	/**
	 *
	 */
	virtual const char* debug_name() {
		return "perform-interruption";
	}

};

#endif
