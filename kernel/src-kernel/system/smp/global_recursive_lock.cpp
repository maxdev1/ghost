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

#include <system/smp/global_recursive_lock.hpp>
#include <logger/logger.hpp>
#include <system/system.hpp>

/**
 *
 */
void g_global_recursive_lock::lock() {

#if G_DEBUG_LOCKS_DEADLOCKING
	uint32_t deadlockCounter = 0;
#endif

	while (!__sync_bool_compare_and_swap(&atom, 0, 1)) {
		if (owner == g_system::currentProcessorId()) {
			++depth;
			return;
		}
		asm("pause");

#if G_DEBUG_LOCKS_DEADLOCKING
		++deadlockCounter;
		if (deadlockCounter > 100000000) {
			g_log_info("%! looks like a deadlock", "rlock");
		}
#endif
	}
	owner = g_system::currentProcessorId();
	depth = 0;
}

/**
 *
 */
void g_global_recursive_lock::unlock() {
	if (depth > 0) {
		--depth;
		return;
	}
	owner = -1;
	atom = 0;
}

/**
 *
 */
bool g_global_recursive_lock::isLocked() {
	return atom;
}

