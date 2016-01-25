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
	uint8_t* atom_1;
	uint8_t* atom_2;
	bool set_on_finish;

public:

	/**
	 *
	 */
	g_waiter_atomic_wait(uint8_t* atom_1, uint8_t* atom_2, bool set_on_finish) :
			atom_1(atom_1), atom_2(atom_2), set_on_finish(set_on_finish) {
	}

	/**
	 *
	 */
	virtual bool checkWaiting(g_thread* task) {

		bool keepWaiting = *atom_1 && (!atom_2 || *atom_2);

		// once waiting is finished, set the atom if required
		if (!keepWaiting && set_on_finish) {
			*atom_1 = true;
			if (atom_2) {
				*atom_2 = true;
			}
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
