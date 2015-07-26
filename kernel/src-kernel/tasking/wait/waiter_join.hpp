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
 *
 */
class g_waiter_join: public g_waiter {
private:
	uint32_t waitTask;

public:

	/**
	 *
	 */
	g_waiter_join(uint32_t _waitTask) {
		waitTask = _waitTask;
	}

	/**
	 *
	 */
	virtual bool checkWaiting(g_thread* task) {

		g_thread* other = g_tasking::getTaskById(waitTask);
		return other != 0 && other->alive;
	}

	/**
	 *
	 */
	virtual const char* debug_name() {
		return "join";
	}

};

#endif
