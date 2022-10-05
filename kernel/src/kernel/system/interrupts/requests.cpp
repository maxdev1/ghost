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

#include "ghost/memory.h"

#include "kernel/calls/syscall.hpp"
#include "kernel/filesystem/filesystem.hpp"
#include "kernel/ipc/pipes.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/tasking/tasking_memory.hpp"
#include "shared/logger/logger.hpp"

g_irq_device* devices[256] = {0};

g_irq_device* requestsGetIrqDevice(uint8_t irq)
{
	g_irq_device* device = devices[irq];
	if(device)
		return device;

	g_fs_node* node;
	if(filesystemCreatePipe(true, &node) != G_FS_PIPE_SUCCESSFUL)
	{
		logInfo("%! failed to create IO pipe for IRQ %i", "requests", irq);
		return nullptr;
	}

	device = (g_irq_device*) heapAllocate(sizeof(g_irq_device));
	device->node = node;
	devices[irq] = device;
	return device;
}

void requestsWriteToIrqDevice(g_task* task, uint8_t irq)
{
	g_irq_device* device = requestsGetIrqDevice(irq);
	if(!device)
		return;

	uint8_t buf[1];
	buf[0] = irq;
	int64_t len;

	// TODO Check status? Wake target task?
	filesystemWrite(device->node, buf, 0, 1, &len);
}
