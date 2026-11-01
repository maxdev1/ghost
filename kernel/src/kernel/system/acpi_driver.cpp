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

#include "kernel/system/acpi_driver.hpp"
#include "kernel/ipc/message_queues.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/memory/memory.hpp"
#include "kernel/tasking/clock.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/tasking_directory.hpp"

g_tid acpiDriverGetService()
{
	auto id = taskingDirectoryGet(G_ACPI_DRIVER_NAME);
	auto task = taskingAcquireTask(id);
	bool trusted = task && task->securityLevel <= G_SECURITY_LEVEL_DRIVER &&
				   !memoryAtomicLoad(&task->terminationRequested, G_MEMORY_ORDER_ACQUIRE);
	taskingReleaseTask(task);
	return trusted ? id : G_TID_NONE;
}

static void acpiDriverSleep(uint64_t milliseconds)
{
	auto task = taskingGetCurrentTask();
	taskingWait(task, __func__,
				[task, milliseconds]() { clockWaitForTime(task->id, clockGetLocal()->time + milliseconds); });
}

static bool acpiDriverSend(g_tid service, g_acpi_request& request, g_message_transaction transaction, uint64_t deadline)
{
	while(messageQueueSend(G_TID_NONE, service, &request, sizeof(request), transaction) !=
		  G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		if(clockGetLocal()->time >= deadline || acpiDriverGetService() != service)
		{
			logWarn("%! ACPI command %i could not be sent before deadline or driver exit", "acpi", request.command);
			return false;
		}
		acpiDriverSleep(1);
	}
	return true;
}

static bool acpiDriverReceive(g_tid service, const g_acpi_request& request, g_message_transaction transaction,
							  uint64_t deadline, g_acpi_response* response)
{
	auto task = taskingGetCurrentTask();
	uint8_t bytes[sizeof(g_message_header) + sizeof(g_acpi_response)];
	while(clockGetLocal()->time < deadline)
	{
		if(acpiDriverGetService() != service)
		{
			logWarn("%! ACPI command %i failed: driver exited while awaiting reply", "acpi", request.command);
			return false;
		}

		auto header = reinterpret_cast<g_message_header*>(bytes);
		if(messageQueueReceive(task->id, header, sizeof(bytes), transaction) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL &&
		   header->sender == service && header->length == sizeof(*response))
		{
			memoryCopy(response, G_MESSAGE_CONTENT(bytes), sizeof(*response));

			if(response->magic != G_ACPI_MAGIC || response->version != G_ACPI_VERSION ||
			   response->command != request.command || response->token != request.token)
			{
				logWarn("%! ACPI command %i failed: invalid driver reply", "acpi", request.command);
				return false;
			}

			if(response->status != G_ACPI_SUCCESSFUL)
				logWarn("%! ACPI command %i failed: driver status %i", "acpi", request.command, response->status);

			return response->status == G_ACPI_SUCCESSFUL;
		}
		acpiDriverSleep(1);
	}
	logWarn("%! ACPI command %i timed out awaiting driver reply", "acpi", request.command);
	return false;
}

static bool acpiDriverTransact(g_acpi_command command, g_power_action action, uint32_t token, g_acpi_response* response)
{
	auto service = acpiDriverGetService();
	if(service == G_TID_NONE)
	{
		logWarn("%! ACPI command %i failed: driver unavailable", "acpi", command);
		return false;
	}

	auto task = taskingGetCurrentTask();
	auto transaction = messageQueueNextTxId();
	auto deadline = clockGetLocal()->time + G_ACPI_TIMEOUT_MS;

	g_acpi_request request{G_ACPI_MAGIC, G_ACPI_VERSION, command, token, task->id, action};
	return acpiDriverSend(service, request, transaction, deadline) &&
		   acpiDriverReceive(service, request, transaction, deadline, response);
}

bool acpiDriverGetInfo(g_acpi_info* info)
{
	auto deadline = clockGetLocal()->time + G_ACPI_TIMEOUT_MS;
	while(acpiDriverGetService() == G_TID_NONE && clockGetLocal()->time < deadline)
		acpiDriverSleep(1);

	g_acpi_response response{};
	if(!acpiDriverTransact(G_ACPI_GET_INFO, G_POWER_SHUTDOWN, 0, &response))
		return false;
	*info = response.info;
	return true;
}

bool acpiDriverPrepare(g_power_action action, uint32_t token, g_acpi_power_plan* plan)
{
	g_acpi_response response{};
	if(!acpiDriverTransact(G_ACPI_PREPARE, action, token, &response))
		return false;
	*plan = response.plan;
	return true;
}

bool acpiDriverCancel(g_power_action action, uint32_t token)
{
	g_acpi_response response{};
	return acpiDriverTransact(G_ACPI_CANCEL, action, token, &response);
}
