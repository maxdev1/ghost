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

#include "kernel/tasking/tasking.hpp"

/**
 * Type of an interrupt handler
 */
typedef struct
{
	g_tid task;
	g_virtual_address handlerAddress;
	g_virtual_address returnAddress;
} g_irq_handler;

/**
 * Handles an interrupt request.
 */
void requestsHandle(g_task* task);

/**
 * Registers a user-space interrupt request handler.
 */
void requestsRegisterHandler(uint8_t irq, g_tid handlerTask, g_virtual_address handlerAddress, g_virtual_address returnAddress);

/**
 * Calls the user-space handler for an IRQ if there is one registered
 * on the given task.
 */
void requestsCallUserspaceHandler(g_task* task, uint8_t irq);

#endif
