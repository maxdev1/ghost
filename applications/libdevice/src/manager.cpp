/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#include "libdevice/manager.hpp"
#include <ghost/messages.h>
#include <ghost/tasks.h>
#include <cstdio>
#include <cstring>

bool deviceManagerRegisterDevice(g_device_type type, g_tid handler, g_device_id* outId)
{
	g_tid managerId = g_task_await_by_name(G_DEVICE_MANAGER_NAME);
	auto tx = g_get_message_tx_id();

	g_device_manager_register_device_request request{};
	request.header.command = G_DEVICE_MANAGER_REGISTER_DEVICE;
	request.handler = handler;
	request.type = type;
	if(!outId || g_send_message_t(managerId, &request, sizeof(request), tx) != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
		return false;

	bool success = false;
	constexpr size_t bufLen = sizeof(g_message_header) + sizeof(g_device_manager_register_device_response);
	uint8_t buf[bufLen];
	if(g_receive_message_t(buf, bufLen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto message = (g_message_header*) buf;
		if(message->sender != managerId || message->length != sizeof(g_device_manager_register_device_response))
			return false;
		auto content = (g_device_manager_register_device_response*) G_MESSAGE_CONTENT(buf);

		if(content->status == G_DEVICE_MANAGER_SUCCESS)
		{
			*outId = content->id;
			success = true;
		}
		else
		{
			klog("failed to register device with status %i", content->status);
		}
	}

	return success;
}

bool deviceManagerListDevices(g_device_id afterId, g_device_manager_list_devices_response* outResponse)
{
	if(!outResponse)
		return false;
	g_tid managerId = g_task_get_by_name(G_DEVICE_MANAGER_NAME);
	if(managerId == G_TID_NONE)
		return false;

	auto tx = g_get_message_tx_id();
	g_device_manager_list_devices_request request{};
	request.header.command = G_DEVICE_MANAGER_LIST_DEVICES;
	request.afterId = afterId;
	if(g_send_message_t(managerId, &request, sizeof(request), tx) != G_MESSAGE_SEND_STATUS_SUCCESSFUL)
		return false;

	static_assert(sizeof(g_device_manager_list_devices_response) <= G_MESSAGE_MAXIMUM_MESSAGE_LENGTH,
	              "device listing page exceeds the IPC message limit");
	uint8_t buffer[sizeof(g_message_header) + sizeof(g_device_manager_list_devices_response)];
	if(g_receive_message_t(buffer, sizeof(buffer), tx) != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		return false;
	auto message = (g_message_header*) buffer;
	if(message->sender != managerId || message->length != sizeof(g_device_manager_list_devices_response))
		return false;
	auto response = (g_device_manager_list_devices_response*) G_MESSAGE_CONTENT(buffer);
	if(response->status != G_DEVICE_MANAGER_SUCCESS || response->count > G_DEVICE_MANAGER_LIST_MAX_DEVICES)
		return false;
	for(uint16_t i = 0; i < response->count; ++i)
	{
		if(response->devices[i].id <= afterId)
			return false;
		afterId = response->devices[i].id;
	}
	memcpy(outResponse, response, sizeof(*outResponse));
	return true;
}
