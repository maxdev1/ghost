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

#ifndef __KERNEL_REQUESTS__
#define __KERNEL_REQUESTS__

#include "kernel/tasking/task.hpp"

void requestsInitialize();

/**
 * @return the IRQ handler task
 */
g_task* requestsGetHandlerTask(uint8_t irq);

/**
 * Sets the registered task for an IRQ.
 */
void requestsSetHandlerTask(uint8_t irq, g_task* task);

/**
 * Registers the task as waiting for this IRQ.
 *
 * @return true if the caller should yield, false if an IRQ was already pending.
 */
bool requestsWaitForIrq(uint8_t irq, g_task* task);

/**
 * Clears a wait that may still be active after a timeout or unrelated wake.
 */
void requestsClearWait(uint8_t irq, g_task* task);

/**
 * Removes any IRQ registrations that still reference a task that is about to be destroyed.
 */
void requestsTaskRemoved(g_task* task);

/**
 * Wakes a registered IRQ handler.
 */
void requestsHandle(g_task* currentTask, uint8_t irq);

#endif
