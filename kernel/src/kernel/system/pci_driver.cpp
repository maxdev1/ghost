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

#include "kernel/system/pci_driver.hpp"
#include <ghost/system/pci.h>
#include "kernel/ipc/message_queues.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking_directory.hpp"

static constexpr uint64_t G_PCI_DRIVER_TIMEOUT = 15000;

static void pciDriverSleep()
{
	auto task = taskingGetCurrentTask();
	taskingWait(task, __func__, [task]() { clockWaitForTime(task->id, clockGetLocal()->time + 1); });
}

static g_tid pciDriverService()
{
	auto id = taskingDirectoryGet(G_PCI_DRIVER_NAME);
	auto task = taskingAcquireTask(id);
	bool trusted = task && task->securityLevel <= G_SECURITY_LEVEL_DRIVER &&
				   !memoryAtomicLoad(&task->terminationRequested, G_MEMORY_ORDER_ACQUIRE);
	taskingReleaseTask(task);
	return trusted ? id : G_TID_NONE;
}

static bool pciDriverSend(g_tid service, void* request, size_t length, int transaction, uint64_t deadline)
{
	auto current = taskingGetCurrentTask();
	while(messageQueueSend(current->id, service, request, length, transaction) != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		if(clockGetLocal()->time >= deadline || pciDriverService() != service)
			return false;

		pciDriverSleep();
	}
	return true;
}

static bool pciDriverReceive(g_tid service, void* response, size_t responseSize, int transaction, uint64_t deadline)
{
	auto current = taskingGetCurrentTask();
	uint8_t buffer[sizeof(g_message_header) + sizeof(g_pci_read_config_response)];
	while(clockGetLocal()->time < deadline)
	{
		auto header = reinterpret_cast<g_message_header*>(buffer);
		auto status = messageQueueReceive(current->id, header, sizeof(buffer), transaction);
		if(status == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL && header->sender == service && header->length == responseSize)
		{
			memoryCopy(response, G_MESSAGE_CONTENT(buffer), responseSize);
			return true;
		}

		if(pciDriverService() != service)
			return false;
		pciDriverSleep();
	}
	return false;
}

/**
 * Sends a bounded transaction to pcidriver and accepts replies only from the
 * selected privileged service. The calling task waits until completion or timeout.
 */
static bool pciDriverTransact(void* request, size_t length, void* response, size_t responseSize)
{
	auto service = pciDriverService();
	if(service == G_TID_NONE)
		return false;

	auto transaction = messageQueueNextTxId();
	auto deadline = clockGetLocal()->time + G_PCI_DRIVER_TIMEOUT;
	return pciDriverSend(service, request, length, transaction, deadline) &&
		   pciDriverReceive(service, response, responseSize, transaction, deadline);
}

bool pciDriverRead(uint32_t device, uint8_t offset, uint8_t width, uint32_t* value)
{
	g_pci_read_config_request request{{G_PCI_READ_CONFIG}, device, offset, width};
	g_pci_read_config_response response{};
	if(!pciDriverTransact(&request, sizeof(request), &response, sizeof(response)) || !response.successful)
		return false;
	*value = response.value;
	return true;
}

bool pciDriverWrite(uint32_t device, uint8_t offset, uint8_t width, uint32_t value)
{
	g_pci_write_config_request request{{G_PCI_WRITE_CONFIG}, device, offset, width, value};
	g_pci_write_config_response response{};
	return pciDriverTransact(&request, sizeof(request), &response, sizeof(response)) && response.successful;
}
