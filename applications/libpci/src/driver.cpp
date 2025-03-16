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

#include "libpci/driver.hpp"
#include <cstdio>
#include <cstring>
#include <ghost/malloc.h>

bool pciDriverListDevices(int* outCount, g_pci_device_data** outData)
{
	g_tid driverTid = g_task_await_by_id(G_PCI_DRIVER_IDENTIFIER);

	g_message_transaction tx = g_get_message_tx_id();

	g_pci_list_devices_request request{};
	request.header.command = G_PCI_LIST_DEVICES;
	g_send_message_t(driverTid, &request, sizeof(request), tx);

	size_t bufLen = sizeof(g_message_header) + sizeof(g_pci_list_devices_count_response);
	uint8_t buf[bufLen];
	if(g_receive_message_t(buf, bufLen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_pci_list_devices_count_response*) G_MESSAGE_CONTENT(buf);

		size_t dataBufLen = sizeof(g_message_header) + response->numDevices * sizeof(g_pci_device_data);
		void* dataBuf = malloc(dataBufLen);
		if(g_receive_message_t(dataBuf, dataBufLen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			*outCount = response->numDevices;
			*outData = (g_pci_device_data*) G_MESSAGE_CONTENT(dataBuf);
			return true;
		}
	}

	return false;
}

void pciDriverFreeDeviceList(g_pci_device_data* deviceList)
{
	free(((uint8_t*) deviceList - sizeof(g_message_header)));
}

bool pciDriverReadConfig(g_pci_device_address address, uint8_t offset, int bytes, uint32_t* outValue)
{
	g_tid driverTid = g_task_await_by_id(G_PCI_DRIVER_IDENTIFIER);

	g_message_transaction tx = g_get_message_tx_id();

	g_pci_read_config_request request{};
	request.header.command = G_PCI_READ_CONFIG;
	request.deviceAddress = address;
	request.offset = offset;
	request.bytes = bytes;
	g_send_message_t(driverTid, &request, sizeof(request), tx);

	bool success = false;
	size_t bufLen = sizeof(g_message_header) + sizeof(g_pci_read_config_response);
	uint8_t buf[bufLen];
	if(g_receive_message_t(buf, bufLen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_pci_read_config_response*) G_MESSAGE_CONTENT(buf);
		success = response->successful;
		*outValue = response->value;
	}

	return success;
}

bool pciDriverWriteConfig(g_pci_device_address address, uint8_t offset, int bytes, uint32_t value)
{
	g_tid driverTid = g_task_await_by_id(G_PCI_DRIVER_IDENTIFIER);

	g_message_transaction tx = g_get_message_tx_id();

	g_pci_write_config_request request{};
	request.header.command = G_PCI_WRITE_CONFIG;
	request.deviceAddress = address;
	request.offset = offset;
	request.bytes = bytes;
	request.value = value;
	g_send_message_t(driverTid, &request, sizeof(request), tx);

	bool success = false;
	size_t bufLen = sizeof(g_message_header) + sizeof(g_pci_write_config_response);
	uint8_t buf[bufLen];
	if(g_receive_message_t(buf, bufLen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_pci_read_config_response*) G_MESSAGE_CONTENT(buf);
		success = response->successful;
	}

	return success;
}

bool pciDriverEnableResourceAccess(g_pci_device_address address, bool enabled)
{
	g_tid driverTid = g_task_await_by_id(G_PCI_DRIVER_IDENTIFIER);

	g_message_transaction tx = g_get_message_tx_id();

	g_pci_enable_resource_access_request request{};
	request.header.command = G_PCI_ENABLE_RESOURCE_ACCESS;
	request.deviceAddress = address;
	request.enabled = enabled;
	g_send_message_t(driverTid, &request, sizeof(request), tx);

	bool success = false;
	size_t bufLen = sizeof(g_message_header) + sizeof(g_pci_enable_resource_access_response);
	uint8_t buf[bufLen];
	if(g_receive_message_t(buf, bufLen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_pci_enable_resource_access_response*) G_MESSAGE_CONTENT(buf);
		success = response->successful;
	}
	return success;
}

bool pciDriverReadBAR(g_pci_device_address address, uint8_t bar, uint32_t* outValue)
{
	g_tid driverTid = g_task_await_by_id(G_PCI_DRIVER_IDENTIFIER);

	g_message_transaction tx = g_get_message_tx_id();

	g_pci_read_bar_request request{};
	request.header.command = G_PCI_READ_BAR;
	request.deviceAddress = address;
	request.bar = bar;
	g_send_message_t(driverTid, &request, sizeof(request), tx);

	bool success = false;
	size_t bufLen = sizeof(g_message_header) + sizeof(g_pci_read_bar_response);
	uint8_t buf[bufLen];
	if(g_receive_message_t(buf, bufLen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_pci_read_bar_response*) G_MESSAGE_CONTENT(buf);
		success = response->successful;
		*outValue = response->value;
	}
	return success;
}

bool pciDriverReadBARSize(g_pci_device_address address, uint8_t bar, uint32_t* outValue)
{
	g_tid driverTid = g_task_await_by_id(G_PCI_DRIVER_IDENTIFIER);

	g_message_transaction tx = g_get_message_tx_id();

	g_pci_read_bar_size_request request{};
	request.header.command = G_PCI_READ_BAR_SIZE;
	request.deviceAddress = address;
	request.bar = bar;
	g_send_message_t(driverTid, &request, sizeof(request), tx);

	bool success = false;
	size_t bufLen = sizeof(g_message_header) + sizeof(g_pci_read_bar_size_response);
	uint8_t buf[bufLen];
	if(g_receive_message_t(buf, bufLen, tx) == G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
	{
		auto response = (g_pci_read_bar_size_response*) G_MESSAGE_CONTENT(buf);
		success = response->successful;
		*outValue = response->value;
	}
	return success;
}
