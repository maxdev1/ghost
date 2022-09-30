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

#include "kernel/filesystem/filesystem.hpp"
#include "kernel/tasking/tasking.hpp"

struct g_irq_device
{
	g_fs_node* node;
	g_tid task;
};

/**
 * Calls the user-space handler for an IRQ if there is one registered
 * on the given task.
 */
void requestsWriteToIrqDevice(g_task* task, uint8_t irq);

/**
 * Retrieves (or creates) the IO device for the IRQ.
 */
g_irq_device* requestsGetIrqDevice(uint8_t irq);

#endif
