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

#include "service.hpp"
#include <cstdio>
#include <cstring>
#include <ghost/system.h>
#include <ghost/tasks.h>
#include <uacpi/kernel_api.h>
#include <uacpi/status.h>
#include "platform.hpp"
#include "power.hpp"

/**
 * A preparation keeps its reservation even on failure. The matching kernel
 * cancellation resumes event handling and rolls back any firmware changes.
 */
static g_acpi_status acpiServicePrepare(g_acpi_service* service, const g_acpi_power_request& request, g_tid coordinator,
										g_acpi_power_plan* plan)
{
	if(service->preparedToken)
		return G_ACPI_BUSY;

	service->preparedToken = request.token;
	service->preparedAction = request.action;
	service->preparedCoordinator = coordinator;
	klog("acpi: preparing %s, token %u", request.action == G_POWER_SHUTDOWN ? "shutdown" : "reboot", request.token);

	auto status = acpiPowerPrepare(request.action, plan) ? G_ACPI_SUCCESSFUL : G_ACPI_HARDWARE_ERROR;
	if(status == G_ACPI_SUCCESSFUL)
	{
		if(!acpiPlatformPause(true))
			status = G_ACPI_HARDWARE_ERROR;
		auto drained = uacpi_kernel_wait_for_work_completion();
		if(drained != UACPI_STATUS_OK)
		{
			klog("acpi: drain work after pausing IRQs failed: %s (%u)", uacpi_status_to_string(drained),
				 unsigned(drained));
			status = G_ACPI_HARDWARE_ERROR;
		}
	}

	klog("acpi: preparation status %u, hardware writes %u", unsigned(status), unsigned(plan->count));
	return status;
}

static g_acpi_status acpiServiceCancel(g_acpi_service* service, const g_acpi_power_request& request, g_tid coordinator)
{
	if(service->preparedToken != request.token || service->preparedAction != request.action ||
	   service->preparedCoordinator != coordinator)
		return G_ACPI_NOT_PERMITTED;

	bool resumed = acpiPlatformPause(false);
	acpiPowerCancel(service->preparedAction);
	service->preparedToken = 0;
	service->preparedCoordinator = G_TID_NONE;
	return resumed ? G_ACPI_SUCCESSFUL : G_ACPI_HARDWARE_ERROR;
}

template<typename Response>
static void acpiServiceReply(g_tid receiver, g_message_transaction transaction, Response response)
{
	auto deadline = g_millis() + G_ACPI_TIMEOUT_MS;
	while(g_send_message_tm(receiver, &response, sizeof(response), transaction, G_MESSAGE_SEND_MODE_NON_BLOCKING) !=
				  G_MESSAGE_SEND_STATUS_SUCCESSFUL &&
		  g_millis() < deadline)
	{
		g_sleep(1);
	}
}

void acpiServiceReceive(g_acpi_service* service, const g_message_header* message, const void* content)
{
	if(message->length < sizeof(g_acpi_header))
		return;

	g_acpi_header header;
	memcpy(&header, content, sizeof(header));
	if(header.magic != G_ACPI_MAGIC || header.version != G_ACPI_VERSION)
		return;

	if(header.command == G_ACPI_GET_INFO)
	{
		if(message->length != sizeof(g_acpi_get_info_request) || message->sender == G_TID_NONE)
			return;

		g_acpi_get_info_response response{{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_GET_INFO, G_ACPI_SUCCESSFUL},
										  service->info};
		acpiServiceReply(message->sender, message->transaction, response);
		return;
	}

	if((header.command != G_ACPI_PREPARE && header.command != G_ACPI_CANCEL) ||
	   message->length != sizeof(g_acpi_power_request))
		return;

	g_acpi_power_request request;
	memcpy(&request, content, sizeof(request));

	auto receiver = message->sender == G_TID_NONE ? request.coordinator : message->sender;
	if(receiver == G_TID_NONE)
		return;

	bool permitted = message->sender == G_TID_NONE && request.token && request.action <= G_POWER_REBOOT;

	if(header.command == G_ACPI_PREPARE)
	{
		g_acpi_prepare_response response{
				{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_PREPARE, G_ACPI_NOT_PERMITTED}, request.token, {}};
		if(permitted)
			response.header.status = acpiServicePrepare(service, request, receiver, &response.plan);

		acpiServiceReply(receiver, message->transaction, response);
	}
	else
	{
		g_acpi_cancel_response response{{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_CANCEL, G_ACPI_NOT_PERMITTED},
										request.token};
		if(permitted)
			response.header.status = acpiServiceCancel(service, request, receiver);

		acpiServiceReply(receiver, message->transaction, response);
	}
}
