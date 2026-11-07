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

#include <cstring>
#include <ghost.h>
#include <ghost/system/acpi.h>
#include <libacpi/acpi.hpp>

static g_acpi_status acpiSendInfoRequest(g_tid service, g_message_transaction transaction, uint64_t deadline)
{
	g_acpi_get_info_request request{{G_ACPI_MAGIC, G_ACPI_VERSION, G_ACPI_GET_INFO}};
	while(g_send_message_tm(service, &request, sizeof(request), transaction, G_MESSAGE_SEND_MODE_NON_BLOCKING) !=
		  G_MESSAGE_SEND_STATUS_SUCCESSFUL)
	{
		if(g_millis() >= deadline)
			return G_ACPI_TIMED_OUT;

		g_sleep(1);
	}

	return G_ACPI_SUCCESSFUL;
}

static g_acpi_status acpiReceiveInfo(g_tid service, g_message_transaction transaction, uint64_t deadline,
									 g_acpi_info* info)
{
	uint8_t bytes[sizeof(g_message_header) + sizeof(g_acpi_get_info_response)];
	while(g_millis() < deadline)
	{
		if(g_task_get_by_name(G_ACPI_DRIVER_NAME) != service)
			return G_ACPI_IO_ERROR;

		if(g_receive_message_tm(bytes, sizeof(bytes), transaction, G_MESSAGE_RECEIVE_MODE_NON_BLOCKING) !=
		   G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			g_sleep(1);
			continue;
		}

		auto header = reinterpret_cast<g_message_header*>(bytes);
		if(header->sender != service || header->length != sizeof(g_acpi_get_info_response))
			continue;

		g_acpi_get_info_response response;
		memcpy(&response, G_MESSAGE_CONTENT(bytes), sizeof(response));

		if(response.header.magic != G_ACPI_MAGIC || response.header.version != G_ACPI_VERSION ||
		   response.header.command != G_ACPI_GET_INFO)
			return G_ACPI_IO_ERROR;

		if(response.header.status == G_ACPI_SUCCESSFUL)
			*info = response.info;

		return response.header.status;
	}

	return G_ACPI_TIMED_OUT;
}

g_acpi_status acpiGetInfo(g_acpi_info* info)
{
	if(!info)
		return G_ACPI_INVALID_ARGUMENT;

	*info = {};

	auto service = g_task_get_by_name(G_ACPI_DRIVER_NAME);
	if(service == G_TID_NONE)
		return G_ACPI_NOT_SUPPORTED;

	auto transaction = g_get_message_tx_id();
	auto deadline = g_millis() + G_ACPI_TIMEOUT_MS;

	auto status = acpiSendInfoRequest(service, transaction, deadline);
	if(status != G_ACPI_SUCCESSFUL)
		return status;

	return acpiReceiveInfo(service, transaction, deadline, info);
}

g_acpi_status acpiGetSleepState(uint8_t state, g_acpi_sleep_state* info)
{
	if(!info || state >= 6)
		return G_ACPI_INVALID_ARGUMENT;

	*info = {};
	g_acpi_info firmware{};

	auto status = acpiGetInfo(&firmware);
	if(status != G_ACPI_SUCCESSFUL)
		return status;

	*info = firmware.sleep[state];
	return info->supported ? G_ACPI_SUCCESSFUL : G_ACPI_NOT_SUPPORTED;
}
