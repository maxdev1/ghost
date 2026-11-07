/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef G_KERNEL_INTERRUPTS_IRQ
#define G_KERNEL_INTERRUPTS_IRQ

#include <ghost/system/irq.h>

struct g_task;

/**
 * Initializes IRQ book keeping before interrupt delivery starts on the BSP.
 */
void irqInitialize();

/**
 * Called from the timer interrupt. The BSP restores retired IOAPIC routes
 * once neither interrupt controller has a delivery outstanding.
 */
void irqReapRetired();

/**
 * Claims an exclusive, initially masked route for the task and returns its
 * handle. Only the task that installed the binding may wait on it.
 */
g_irq_status irqInstall(g_task* task, g_irq_source source, uint32_t line, g_irq_handle* out);

/**
 * Returns pending delivery, timeout or release. A zero timeout waits
 * indefinitely. A timeout leaves the route armed as before.
 */
g_irq_status irqWait(g_task* task, g_irq_handle handle, uint32_t timeoutMs);

/**
 * The owning process may arm or mask the route. Complete arms it initially
 * and rearms it after the driver has cleared the interrupt source.
 */
g_irq_status irqComplete(g_task* task, g_irq_handle handle);
g_irq_status irqMask(g_task* task, g_irq_handle handle);

/**
 * Invalidates the handle and wakes its waiter. A retiring IOAPIC route
 * retains its physical line and vector until outstanding delivery drains.
 */
g_irq_status irqRelease(g_task* task, g_irq_handle handle);

/**
 * Handles an IRQ offset relative to IDT vector 0x20. Latches delivery,
 * masks level-triggered sources, and sends EOI before waking the waiter.
 */
void irqHandle(g_task* interrupted, uint8_t vector);

/**
 * Releases bindings owned by an exiting task. An abandoned level-triggered
 * source stays masked because the driver may have exited before clearing it.
 */
void irqTaskStopped(g_task* task);

#endif
