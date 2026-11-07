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
	g_interrupt_guard interrupts;
	taskingPrepareWait(task, __func__);
	clockWaitForTime(task->id, clockGetMilliseconds() + milliseconds);

	taskingHandleTerminationRequest();
	taskingYield();
	taskingHandleTerminationRequest();
}

static bool acpiDriverSend(g_tid service, g_acpi_header* request, size_t requestSize,
						   g_message_transaction transaction, uint64_t deadline)
{
	auto sender = request->command == G_ACPI_GET_INFO ? taskingGetCurrentTask()->id : G_TID_NONE;
	while(messageQueueSend(sender, service, request, requestSize, transaction) !=
		  G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		if(clockGetMilliseconds() >= deadline || acpiDriverGetService() != service)
		{
			logWarn("%! ACPI command %i could not be sent before deadline or driver exit", "acpi", request->command);
			return false;
		}

		acpiDriverSleep(1);
	}

	return true;
}

static bool acpiDriverResponseMatches(const g_message_header* message, g_acpi_command command, uint32_t token)
{
	if(message->length < sizeof(g_acpi_response_header))
		return false;

	auto response = (const g_acpi_response_header*) G_MESSAGE_CONTENT(message);
	if(response->magic != G_ACPI_MAGIC || response->version != G_ACPI_VERSION || response->command != command)
		return false;

	switch(command)
	{
		case G_ACPI_GET_INFO:
			return message->length == sizeof(g_acpi_get_info_response);
		case G_ACPI_PREPARE:
			return message->length == sizeof(g_acpi_prepare_response) &&
				   ((const g_acpi_prepare_response*) response)->token == token;
		case G_ACPI_CANCEL:
			return message->length == sizeof(g_acpi_cancel_response) &&
				   ((const g_acpi_cancel_response*) response)->token == token;
		default:
			return false;
	}
}

static bool acpiDriverReceive(g_tid service, g_acpi_command command, uint32_t token,
							  g_message_transaction transaction, uint64_t deadline,
							  g_message_header* message, size_t responseSize)
{
	auto task = taskingGetCurrentTask();
	while(clockGetMilliseconds() < deadline)
	{
		if(acpiDriverGetService() != service)
		{
			logWarn("%! ACPI command %i failed: driver exited while awaiting reply", "acpi", command);
			return false;
		}

		if(messageQueueReceive(task->id, message, sizeof(g_message_header) + responseSize, transaction) ==
				   G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL &&
		   message->sender == service && message->length == responseSize)
		{
			if(!acpiDriverResponseMatches(message, command, token))
			{
				logWarn("%! ACPI command %i failed: invalid driver reply", "acpi", command);
				return false;
			}

			auto response = (g_acpi_response_header*) G_MESSAGE_CONTENT(message);
			if(response->status != G_ACPI_SUCCESSFUL)
				logWarn("%! ACPI command %i failed: driver status %i", "acpi", command, response->status);

			return response->status == G_ACPI_SUCCESSFUL;
		}

		acpiDriverSleep(1);
	}

	logWarn("%! ACPI command %i timed out awaiting driver reply", "acpi", command);
	return false;
}

static bool acpiDriverTransact(g_acpi_header* request, size_t requestSize,
							   g_message_header* message, size_t responseSize, uint32_t token = 0)
{
	auto service = acpiDriverGetService();
	if(service == G_TID_NONE)
	{
		logWarn("%! ACPI command %i failed: driver unavailable", "acpi", request->command);
		return false;
	}

	auto transaction = messageQueueNextTxId();
	auto deadline = clockGetMilliseconds() + G_ACPI_TIMEOUT_MS;
	return acpiDriverSend(service, request, requestSize, transaction, deadline) &&
		   acpiDriverReceive(service, request->command, token, transaction, deadline, message, responseSize);
}

bool acpiDriverGetInfo(g_acpi_info* info)
{
	auto deadline = clockGetMilliseconds() + G_ACPI_TIMEOUT_MS;
	while(acpiDriverGetService() == G_TID_NONE && clockGetMilliseconds() < deadline)
		acpiDriverSleep(1);

	g_acpi_get_info_request request{{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_GET_INFO}};
	alignas(g_message_header) uint8_t bytes[sizeof(g_message_header) + sizeof(g_acpi_get_info_response)];
	auto message = (g_message_header*) bytes;
	if(!acpiDriverTransact(&request.header, sizeof(request), message, sizeof(g_acpi_get_info_response)))
		return false;

	auto response = (g_acpi_get_info_response*) G_MESSAGE_CONTENT(message);
	*info = response->info;
	return true;
}

bool acpiDriverPrepare(g_power_action action, uint32_t token, g_acpi_power_plan* plan)
{
	g_acpi_prepare_request request{{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_PREPARE}, token, taskingGetCurrentTask()->id, action};
	alignas(g_message_header) uint8_t bytes[sizeof(g_message_header) + sizeof(g_acpi_prepare_response)];
	auto message = (g_message_header*) bytes;
	if(!acpiDriverTransact(&request.header, sizeof(request), message, sizeof(g_acpi_prepare_response), token))
		return false;

	auto response = (g_acpi_prepare_response*) G_MESSAGE_CONTENT(message);
	*plan = response->plan;
	return true;
}

bool acpiDriverCancel(g_power_action action, uint32_t token)
{
	g_acpi_cancel_request request{{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_CANCEL}, token, taskingGetCurrentTask()->id, action};
	alignas(g_message_header) uint8_t bytes[sizeof(g_message_header) + sizeof(g_acpi_cancel_response)];
	return acpiDriverTransact(&request.header, sizeof(request), (g_message_header*) bytes,
							  sizeof(g_acpi_cancel_response), token);
}
