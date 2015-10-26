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

#include <tasking/tasking.hpp>

#include <kernel.hpp>
#include <logger/logger.hpp>
#include <system/cpu_state.hpp>
#include <tasking/scheduling/scheduler.hpp>
#include <system/system.hpp>
#include <tasking/thread_manager.hpp>
#include <debug/debug_interface_kernel.hpp>

static g_scheduler** schedulers;

/**
 * 
 */
void g_tasking::initialize() {

	// Create space for all schedulers
	uint32_t numCores = g_system::getCpuCount();
	schedulers = new g_scheduler*[numCores];

	// Zero
	for (uint32_t i = 0; i < numCores; i++) {
		schedulers[i] = 0;
	}

}

/**
 * 
 */
void g_tasking::enableForThisCore() {

	uint32_t coreId = g_system::getCurrentCoreId();
	schedulers[coreId] = new g_scheduler(coreId);
	g_log_info("%! scheduler installed on core %i", "tasking", coreId);

}

/**
 * 
 */
g_cpu_state* g_tasking::switchTask(g_cpu_state* cpuState) {

	// Get scheduler for this core
	uint32_t coreId = g_system::getCurrentCoreId();
	g_scheduler* scheduler = schedulers[coreId];

	// Check for errors
	if (scheduler == 0) {
		g_kernel::panic("%! no scheduler for core %i", "scheduler", coreId);
	}

	// Let scheduler do his work
	return scheduler->switchTask(cpuState);
}

/**
 * 
 */
g_thread* g_tasking::fork() {

	g_thread* clone = 0;

	g_thread* current = getCurrentThread();

	// TODO forking in threads.
	if (current == current->process->main) {
		if (current) {
			clone = g_thread_manager::fork(current);
			if (clone) {
				addTask(clone);
			}
		}
	} else {
		g_log_info("%! can't fork anything but the main thread", "todo");
	}

	return clone;
}

/**
 * 
 */
void g_tasking::addTask(g_thread* t, bool enforceCurrentCore) {

	g_scheduler* target = 0;

	// Used by AP's for the idle binary
	if (enforceCurrentCore) {
		target = getCurrentScheduler();

	} else {
		// Find core with lowest load
		g_scheduler* lowest = 0;
		uint32_t lowestLoad = 0;

		for (uint32_t i = 0; i < g_system::getCpuCount(); i++) {
			g_scheduler* sched = schedulers[i];
			if (sched) {
				// Check if load is lower than others
				uint32_t load = sched->getLoad();
				if (lowest == 0 || load < lowestLoad) {
					lowest = sched;
					lowestLoad = load;
				}
			}
		}

		target = lowest;
	}

	// Error check
	if (target == 0) {
		g_kernel::panic("%! couldn't find scheduler to add task to", "tasking");
	}

	// Assign task to scheduler
	target->add(t);
}

/**
 * Returns the current scheduler on the current core
 */
g_scheduler* g_tasking::getCurrentScheduler() {

	uint32_t coreId = g_system::getCurrentCoreId();
	g_scheduler* sched = schedulers[coreId];

	// Error check
	if (sched == 0) {
		g_kernel::panic("%! no scheduler exists for core %i", "tasking", coreId);
	}

	return sched;
}

/**
 * 
 */
g_thread* g_tasking::getCurrentThread() {

	return getCurrentScheduler()->getCurrent();
}

/**
 *
 */
g_thread* g_tasking::getTaskById(uint32_t id) {

	for (uint32_t i = 0; i < g_system::getCpuCount(); i++) {
		g_scheduler* sched = schedulers[i];

		if (sched) {
			g_thread* task = sched->getTaskById(id);
			if (task) {
				return task;
			}
		}
	}

	return 0;
}

/**
 *
 */
g_thread* g_tasking::getTaskByIdentifier(const char* identifier) {

	for (uint32_t i = 0; i < g_system::getCpuCount(); i++) {
		g_scheduler* sched = schedulers[i];

		if (sched) {
			g_thread* task = sched->getTaskByIdentifier(identifier);
			if (task) {
				return task;
			}
		}
	}

	return 0;
}

/**
 *
 */
bool g_tasking::registerTaskForIdentifier(g_thread* task, const char* newIdentifier) {

	// Check if someone else has this identifier
	g_thread* existing = getTaskByIdentifier(newIdentifier);
	if (existing) {
		g_log_warn("%! task %i could not be registered as '%s', name is used by %i", "tasking", task->id, newIdentifier, existing->id);
		return false;
	}

	// Set the identifier
	task->setIdentifier(newIdentifier);

	G_DEBUG_INTERFACE_TASK_SET_IDENTIFIER(task->id, newIdentifier);
	g_log_debug("%! task %i registered as '%s'", "tasking", task->id, newIdentifier);
	return true;
}

/**
 *
 */
bool g_tasking::killAllThreadsOf(g_process* process) {

	bool still_has_living_threads = true;

	for (uint32_t i = 0; i < g_system::getCpuCount(); i++) {
		g_scheduler* sched = schedulers[i];

		if (sched && sched->killAllThreadsOf(process)) {
			still_has_living_threads = false;
		}
	}

	return still_has_living_threads;
}

