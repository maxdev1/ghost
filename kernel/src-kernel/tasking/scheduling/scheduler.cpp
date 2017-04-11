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

#include <tasking/scheduling/scheduler.hpp>
#include <tasking/wait/waiter_sleep.hpp>
#include <system/interrupts/lapic.hpp>
#include <logger/logger.hpp>
#include <tasking/thread_manager.hpp>
#include <memory/address_space.hpp>
#include <memory/gdt/gdt_manager.hpp>
#include <utils/string.hpp>
#include <kernel.hpp>
#include "debug/debug_interface_kernel.hpp"

/**
 *
 */
g_scheduler::g_scheduler(uint32_t coreId) :
		milliseconds(0), coreId(coreId), wait_queue(0), run_queue(0), idle_entry(0), current_entry(0) {
}

/**
 *
 */
void g_scheduler::add(g_thread* t) {

	// set the scheduler on the task
	t->scheduler = this;

	// the idle task is not added to a queue
	if (t->priority == G_THREAD_PRIORITY_IDLE) {
		g_task_entry* entry = new g_task_entry;
		entry->value = t;
		entry->next = 0;
		idle_entry = entry;

	} else {
		// add task to run queue
		g_task_entry* entry = new g_task_entry;
		entry->value = t;
		entry->next = run_queue;
		run_queue = entry;
	}

	g_log_debug("%! task %i assigned to core %i", "scheduler", t->id, coreId);
}

/**
 *
 */
void g_scheduler::remove_threads(g_process* process) {

	// set all threads in run queue to dead
	auto entry = run_queue;
	while (entry) {
		auto next = entry->next;
		if (entry->value->process->main->id == process->main->id) {
			_remove(entry->value);
		}

		entry = next;
	}

	// set all threads in wait queue to dead
	entry = wait_queue;
	while (entry) {
		auto next = entry->next;
		if (entry->value->process->main->id == process->main->id) {
			_remove(entry->value);
		}
		entry = next;
	}
}

/**
 *
 */
g_thread* g_scheduler::lastThread() {
	if (current_entry) {
		return current_entry->value;
	}
	return nullptr;
}

/**
 *
 */
uint32_t g_scheduler::calculateLoad() {

	uint32_t load = 0;

	// TODO improve load calculation
	g_task_entry* entry = run_queue;
	while (entry) {
		++load;
		entry = entry->next;
	}
	entry = wait_queue;
	while (entry) {
		++load;
		entry = entry->next;
	}

	return load;
}

/**
 *
 */
void g_scheduler::updateMilliseconds() {
	milliseconds += APIC_MILLISECONDS_PER_TICK;

#if G_DEBUG_THREAD_DUMPING
	// debug dump
	static uint64_t last_debugout_millis = 0;
	if (milliseconds - last_debugout_millis > 10000) {
		last_debugout_millis = milliseconds;

		g_log_info("----------------");
		g_task_entry* entry = run_queue;
		while (entry) {
			g_thread* thr = entry->value;
			g_log_info("%# running - %i:%i, eip: %h, waiter: %s, name: %s, rounds: %i", thr->process->main->id, thr->id, thr->cpuState->eip,
					(thr->waitManager == nullptr ? "-" : thr->waitManager->debug_name()), (thr->getIdentifier() == 0 ? "-" : thr->getIdentifier()),
					thr->rounds);
			entry = entry->next;
		}
		entry = wait_queue;
		while (entry) {
			g_thread* thr = entry->value;
			g_log_info("%# waiting - %i:%i, eip: %h, waiter: %s, name: %s, rounds: %i", thr->process->main->id, thr->id, thr->cpuState->eip,
					(thr->waitManager == nullptr ? "-" : thr->waitManager->debug_name()), (thr->getIdentifier() == 0 ? "-" : thr->getIdentifier()),
					thr->rounds);
			entry = entry->next;
		}
	}
#endif

#if G_DEBUG_INTERFACE_MODE == G_DEBUG_INTERFACE_MODE_FULL
	static uint64_t lastProcessorTimeUpdate = 0;
	if (milliseconds - lastProcessorTimeUpdate > 500) {
		lastProcessorTimeUpdate = milliseconds;

		g_task_entry* entry = run_queue;
		while (entry) {
			g_thread* thr = entry->value;
			G_DEBUG_INTERFACE_TASK_SET_ROUNDS(thr->id, thr->rounds);
			thr->rounds = 0;
			entry = entry->next;
		}
		entry = wait_queue;
		while (entry) {
			g_thread* thr = entry->value;
			G_DEBUG_INTERFACE_TASK_SET_ROUNDS(thr->id, thr->rounds);
			thr->rounds = 0;
			entry = entry->next;
		}
	}
#endif
}

/**
 *
 */
uint64_t g_scheduler::getMilliseconds() {
	return milliseconds;
}

/**
 *
 */
g_thread* g_scheduler::save(g_processor_state* cpuState) {

	// store processor state in current task
	if (current_entry) {
		current_entry->value->cpuState = cpuState;
		return current_entry->value;
	}

	// no last thread? do scheduling
	return schedule();
}

/**
 *
 */
g_thread* g_scheduler::schedule() {

	// store which entry is running
	g_task_entry* running_entry = current_entry;

	// process wait queue (overwrites current entry)
	_processWaitQueue();

	// continue scheduling there
	current_entry = running_entry;

	// select next task to run
	while (true) {
		// when scheduling for the first time, there's not current task
		if (current_entry == 0) {
			current_entry = run_queue;

		} else {
			// select next in run queue
			current_entry = current_entry->next;
			if (current_entry == 0) {
				current_entry = run_queue;
			}
		}

		// no task in run queue? select idle thread
		if (current_entry == 0) {
			current_entry = idle_entry;

			if (current_entry == 0) {
				g_kernel::panic("%! idle thread does not exist on core %i", "scheduler", coreId);
			}
		}

		// sanity check
		if (current_entry->value->waitManager) {
			g_kernel::panic("task %i is in run queue had wait manager '%s'", current_entry->value->id, current_entry->value->waitManager->debug_name());
		}

		// try to switch
		_applyContextSwitch(current_entry->value);

		// remove it task is no more alive
		if (!_checkAliveState(current_entry->value)) {
			current_entry = 0;
			continue;
		}

		// task was successfully selected & switched to
		break;
	}

	// finish the switch
	_finishSwitch(current_entry->value);
	++current_entry->value->rounds;

	return current_entry->value;
}

/**
 *
 */
void g_scheduler::_applyContextSwitch(g_thread* thread) {

	// switch to the address space
	g_address_space::switch_to_space(thread->process->pageDirectory);

	// set ESP0 in the TSS
	g_gdt_manager::setTssEsp0(thread->kernelStackEsp0);
}

/**
 *
 */
bool g_scheduler::_checkAliveState(g_thread* thread) {

	// Eliminate if dead
	if (!thread->alive) {

		if (thread->type == G_THREAD_TYPE_MAIN) {
			g_tasking::remove_threads(thread->process);
		} else {
			_remove(thread);
		}

		return false;
	}

	return true;
}

/**
 *
 */
void g_scheduler::_finishSwitch(g_thread* thread) {

	// write user thread address to GDT
	g_gdt_manager::setUserThreadAddress(thread->user_thread_addr);

	// set GS of thread to user pointer segment
	thread->cpuState->gs = 0x30;
}

/**
 *
 */
void g_scheduler::_remove(g_thread* thread) {

	// remove from run queue
	g_task_entry* entry = _removeFromQueue(&run_queue, thread);

	// if it was not in run queue, get it from wait queue
	if (entry == 0) {
		entry = _removeFromQueue(&wait_queue, thread);
	}

	// if it was in neither queues, thats weird
	if (entry == 0) {
		g_log_warn("%! failed to properly delete thread %i, was not assigned to a queue", "scheduler", thread->id);
		return;
	}

	// delete entry and task
	delete entry;
	g_thread_manager::deleteTask(thread);
}

/**
 * 
 */
void g_scheduler::_printDeadlockWarning() {

	char* taskName = (char*) "?";
	if (current_entry->value->getIdentifier() != 0) {
		taskName = (char*) current_entry->value->getIdentifier();
	}

	g_log_debug("%! thread %i (process %i, named '%s') waits for '%s'", "deadlock-detector", current_entry->value->id, current_entry->value->process->main->id,
			taskName, current_entry->value->waitManager->debug_name());
}

/**
 *
 */
g_thread* g_scheduler::getTaskById(g_tid id) {

	g_thread* thr = 0;

	g_task_entry* entry = run_queue;
	while (entry) {
		if (entry->value->alive && entry->value->id == id) {
			thr = entry->value;
			break;
		}
		entry = entry->next;
	}

	if (thr == 0) {
		entry = wait_queue;
		while (entry) {
			if (entry->value->alive && entry->value->id == id) {
				thr = entry->value;
				break;
			}
			entry = entry->next;
		}
	}

	return thr;
}

/**
 *
 */
g_thread* g_scheduler::getTaskByIdentifier(const char* identifier) {

	g_thread* thr = 0;

	g_task_entry* entry = run_queue;
	while (entry) {
		if (entry->value->alive) {
			const char* taskIdentifier = entry->value->getIdentifier();
			if (taskIdentifier != 0 && g_string::equals(taskIdentifier, identifier)) {
				thr = entry->value;
				break;
			}
		}
		entry = entry->next;
	}

	if (thr == 0) {
		entry = wait_queue;
		while (entry) {

			if (entry->value->alive) {
				const char* taskIdentifier = entry->value->getIdentifier();
				if (taskIdentifier != 0 && g_string::equals(taskIdentifier, identifier)) {
					thr = entry->value;
					break;
				}
			}
			entry = entry->next;
		}
	}

	return thr;
}

/**
 *
 */
g_task_entry* g_scheduler::_removeFromQueue(g_task_entry** queue_head, g_thread* thread) {

	g_task_entry* removed_entry = 0;
	g_task_entry* entry = *queue_head;

	// if queue is empty, entry can't be removed
	if (entry == 0) {
		return 0;
	}

	// if it's the head of the queue, replace it
	if (entry->value == thread) {
		removed_entry = entry;
		*queue_head = removed_entry->next;

	} else {
		// otherwise, find entry before it and replace it
		g_task_entry* previous = 0;
		while (entry) {
			if (entry->value == thread) {
				removed_entry = entry;
				previous->next = removed_entry->next;
				break;
			}
			previous = entry;
			entry = entry->next;
		}
	}

	return removed_entry;
}

/**
 *
 */
void g_scheduler::moveToRunQueue(g_thread* thread) {

	g_task_entry* move_entry = _removeFromQueue(&wait_queue, thread);

	if (move_entry == 0) {
		// entry is already in run queue
		return;
	}

	// put to start of run queue
	move_entry->next = run_queue;
	run_queue = move_entry;

}

/**
 *
 */
void g_scheduler::moveToWaitQueue(g_thread* thread) {

	g_task_entry* move_entry = _removeFromQueue(&run_queue, thread);

	if (move_entry == 0) {
		// entry is already in wait queue
		return;
	}

	// put to start of wait queue
	move_entry->next = wait_queue;
	wait_queue = move_entry;

	// may no more be the running entry
	if (move_entry == current_entry) {
		current_entry = nullptr;
	}

}

/**
 *
 */
void g_scheduler::increaseWaitPriority(g_thread* thread) {

	// remove entry from wait queue
	g_task_entry* entry = _removeFromQueue(&wait_queue, thread);

	// put it on top of it
	if (entry) {
		entry->next = wait_queue;
		wait_queue = entry;
	}

}

/**
 *
 */
void g_scheduler::_processWaitQueue() {

	// process wait queue
	current_entry = wait_queue;

	while (current_entry) {
		g_task_entry* next = current_entry->next;

		// switch to tasks space
		_applyContextSwitch(current_entry->value);

		// remove it if its dead
		if (_checkAliveState(current_entry->value)) {
			// check its waiting state
			_checkWaitingState(current_entry->value);
		}

		current_entry = next;
	}
}

/**
 *
 */
void g_scheduler::_checkWaitingState(g_thread* thread) {

	// check if task must continue waiting
	if (thread->waitManager != nullptr) {

		if (thread->checkWaiting()) {
			// increase wait counter for deadlock warnings
			thread->waitCount++;
			if (thread->waitCount % 500000 == 0) {
				_printDeadlockWarning();
			}

		} else {
			// reset wait counter & remove wait handler
			thread->waitCount = 0;
			thread->unwait();
		}
	}
}

/**
 *
 */
uint32_t g_scheduler::count() {

	uint32_t count = 0;

	auto entry = run_queue;
	while (entry) {
		++count;
		entry = entry->next;
	}

	entry = wait_queue;
	while (entry) {
		++count;
		entry = entry->next;
	}

	return count;
}

/**
 *
 */
uint32_t g_scheduler::get_task_ids(g_tid* out, uint32_t len) {

	uint32_t pos = 0;

	auto entry = run_queue;
	while (pos < len && entry) {
		g_log_info("inserting %i at %i (%h)", entry->value->id, pos, &out[pos]);
		out[pos++] = entry->value->id;
		entry = entry->next;
	}

	entry = wait_queue;
	while (pos < len && entry) {
		g_log_info("inserting %i at %i (%h)", entry->value->id, pos, &out[pos]);
		out[pos++] = entry->value->id;
		entry = entry->next;
	}

	return pos;
}
