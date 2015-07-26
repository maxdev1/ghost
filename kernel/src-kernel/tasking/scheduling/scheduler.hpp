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

#ifndef SCHEDULER_HPP_
#define SCHEDULER_HPP_

#include "ghost/stdint.h"
#include <utils/list_entry.hpp>
#include <tasking/thread.hpp>
#include <system/cpu_state.hpp>
#include <system/smp/global_recursive_lock.hpp>

/**
 *
 */
class g_scheduler {
private:
	uint64_t milliseconds;

	g_global_recursive_lock taskListLock;
	g_list_entry<g_thread*>* taskList;
	g_list_entry<g_thread*>* current;

	uint32_t coreId;

	void selectNext();
	bool applySwitch();

	bool handleWaiting();
	void deleteCurrent();

public:
	g_scheduler(uint32_t coreId);

	void lock();
	void unlock();

	g_cpu_state* switchTask(g_cpu_state* cpuState);
	void add(g_thread* t);

	uint32_t getLoad();

	g_thread* getCurrent();
	g_thread* getTaskById(g_tid id);
	g_thread* getTaskByIdentifier(const char* identifier);

	/**
	 * Sets all threads of the given process to !alive and returns
	 * whether there where any threads that were still alive.
	 */
	bool killAllThreadsOf(g_process* process);

	void updateMilliseconds();
	void sleep(g_thread* process, uint64_t millis);
	uint64_t getMilliseconds();

	void print_waiter_deadlock_warning();
};

#endif
