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

#ifndef GHOST_MULTITASKING_WAIT_MANAGER_ATOMIC_WAIT
#define GHOST_MULTITASKING_WAIT_MANAGER_ATOMIC_WAIT

#include <tasking/wait/waiter.hpp>
#include <logger/logger.hpp>

/**
 *
 */
class g_waiter_atomic_wait: public g_waiter {
private:
	bool* atom;
	bool set_on_finish;

public:

	/**
	 *
	 */
	g_waiter_atomic_wait(bool* atom, bool set_on_finish) :
			atom(atom), set_on_finish(set_on_finish) {
	}

	/**
	 *
	 */
	virtual bool checkWaiting(g_thread* task) {

		bool keepWaiting = *atom;

		// once waiting is finished, set the atom if required
		if (!keepWaiting && set_on_finish) {
			*atom = true;
		}

		return keepWaiting;
	}

	/**
	 *
	 */
	virtual const char* debug_name() {
		return "atomic-wait";
	}

};

#endif
