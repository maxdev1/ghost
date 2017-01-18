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
	g_syscall_atomic_lock* data;

	g_scheduler* timing_scheduler = 0;
	uint64_t time_start = 0;

public:

	/**
	 *
	 */
	g_waiter_atomic_wait(g_syscall_atomic_lock* data) :
			data(data) {
	}

	/**
	 *
	 */
	g_waiter_atomic_wait(g_syscall_atomic_lock* data, g_scheduler* timing_scheduler) :
			data(data), timing_scheduler(timing_scheduler) {
		time_start = timing_scheduler->getMilliseconds();
	}

	/**
	 *
	 */
	virtual bool checkWaiting(g_thread* task) {

		// check timeout first
		if (data->has_timeout && (timing_scheduler->getMilliseconds() - time_start > data->timeout)) {
			// timeout exceeded
			data->timed_out = true;
			return false;
		}

		// once waiting is finished, set the atom if required
		bool keep_wait = *data->atom_1 && (!data->atom_2 || *data->atom_2);

		if (!keep_wait && data->set_on_finish) {
			*data->atom_1 = true;
			if (data->atom_2) {
				*data->atom_2 = true;
			}
			data->was_set = true;
		}

		return keep_wait;
	}

	/**
	 *
	 */
	virtual const char* debug_name() {
		return "atomic-wait";
	}

};

#endif
