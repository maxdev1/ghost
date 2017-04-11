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
#include <tasking/scheduling/scheduler.hpp>
#include <system/system.hpp>
#include <tasking/thread_manager.hpp>
#include <debug/debug_interface_kernel.hpp>
#include <system/processor_state.hpp>

static g_scheduler** schedulers;

/**
 * 
 */
void g_tasking::initialize() {

	// Create space for all schedulers
	uint32_t numCores = g_system::getNumberOfProcessors();
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

	uint32_t coreId = g_system::currentProcessorId();
	schedulers[coreId] = new g_scheduler(coreId);
	g_log_info("%! scheduler installed on core %i", "tasking", coreId);

}

/**
 * 
 */
void g_tasking::increaseWaitPriority(g_thread* thread) {
	currentScheduler()->increaseWaitPriority(thread);
}

/**
 *
 */
g_thread* g_tasking::save(g_processor_state* cpuState) {
	return currentScheduler()->save(cpuState);
}

/**
 *
 */
g_thread* g_tasking::schedule() {
	return currentScheduler()->schedule();
}

/**
 * 
 */
g_thread* g_tasking::fork(g_thread* current_thread) {

	g_thread* clone = 0;

	// TODO forking in threads.
	if (current_thread == current_thread->process->main) {
		if (current_thread) {
			clone = g_thread_manager::fork(current_thread);
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
		target = currentScheduler();

	} else {
		// Find core with lowest load
		g_scheduler* lowest = 0;
		uint32_t lowestLoad = 0;

		for (uint32_t i = 0; i < g_system::getNumberOfProcessors(); i++) {
			g_scheduler* sched = schedulers[i];
			if (sched) {
				// Check if load is lower than others
				uint32_t load = sched->calculateLoad();
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
g_scheduler* g_tasking::currentScheduler() {

	uint32_t coreId = g_system::currentProcessorId();
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
g_thread* g_tasking::lastThread() {
	return currentScheduler()->lastThread();
}

/**
 *
 */
g_thread* g_tasking::getTaskById(uint32_t id) {

	for (uint32_t i = 0; i < g_system::getNumberOfProcessors(); i++) {
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

	for (uint32_t i = 0; i < g_system::getNumberOfProcessors(); i++) {
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
void g_tasking::remove_threads(g_process* process) {

	uint32_t processors = g_system::getNumberOfProcessors();
	for (uint32_t i = 0; i < processors; i++) {
		g_scheduler* sched = schedulers[i];
		if (sched) {
			sched->remove_threads(process);
		}
	}
}

/**
 *
 */
uint32_t g_tasking::count() {
	uint32_t total = 0;
	uint32_t processors = g_system::getNumberOfProcessors();
	for (uint32_t i = 0; i < processors; i++) {
		g_scheduler* sched = schedulers[i];
		if (sched) {
			total += sched->count();
		}
	}
	return total;
}

/**
 *
 */
uint32_t g_tasking::get_task_ids(g_tid* out, uint32_t len) {

	uint32_t pos = 0;
	g_log_info("start: %h", out);

	uint32_t processors = g_system::getNumberOfProcessors();
	for (uint32_t i = 0; i < processors; i++) {
		g_scheduler* sched = schedulers[i];
		if (sched) {
			pos += sched->get_task_ids(&out[pos], len - pos);
		}
	}

	return pos;
}
