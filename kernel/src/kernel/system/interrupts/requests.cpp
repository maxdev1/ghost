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


#include "kernel/filesystem/filesystem.hpp"
#include "kernel/system/interrupts/requests.hpp"
#include "kernel/system/processor/processor.hpp"
#include "shared/logger/logger.hpp"

static g_irq_device* devices[256] = {0};
static g_mutex devicesLock;

void requestsInitialize()
{
	mutexInitializeNonInterruptible(&devicesLock, __func__);
}

g_irq_device* requestsGetIrqDevice(uint8_t irq)
{
	mutexAcquire(&devicesLock);
	g_irq_device* device = devices[irq];

	if(device)
	{
		mutexRelease(&devicesLock);
		return device;
	}

	// Create a pipe if it doesn't exist yet
	// TODO Not sure if it should be blocking or not...
	g_fs_node* node;
	if(filesystemCreatePipe(false, &node, true) != G_FS_PIPE_SUCCESSFUL)
	{
		logInfo("%! failed to create IO pipe for IRQ %i", "requests", irq);
		return nullptr;
	}

	// Kernel holds write-end of the pipe
	g_fd writeFd;
	g_fs_open_status openStatus = filesystemOpenNodeFd(node, G_FILE_FLAG_MODE_WRITE, G_PID_NONE, &writeFd);
	if(openStatus != G_FS_OPEN_SUCCESSFUL)
	{
		logWarn("%! failed to open IO pipe write end in kernel for IRQ %i", "requests", irq);
	}

	device = (g_irq_device*) heapAllocate(sizeof(g_irq_device));
	device->node = node;
	devices[irq] = device;

	mutexRelease(&devicesLock);
	return device;
}

void requestsWriteToIrqDevice(uint8_t irq)
{
	g_irq_device* device = requestsGetIrqDevice(irq);
	if(!device)
		return;

	uint8_t buf[1];
	buf[0] = irq;
	int64_t len;

	filesystemWrite(device->node, buf, 0, 1, &len);
}
