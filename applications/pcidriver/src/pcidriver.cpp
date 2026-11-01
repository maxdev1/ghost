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

#include "pcidriver.hpp"

#include <cstdio>
#include <libpci/pci.hpp>
#include <libpci/driver.hpp>
#include <ghost.h>
#include <malloc.h>
#include <vector>
#include <utility>
#include <cstring>

g_user_mutex configSpaceLock = g_mutex_initialize();
g_pci_device* deviceList = nullptr;
int deviceCount = 0;
g_user_mutex deviceListLock = g_mutex_initialize();

static std::vector<std::pair<g_pci_device*, uint16_t>> pciPowerCommands;
static uint32_t pciPowerRequest;
static g_tid pciPowerCoordinator = G_TID_NONE;

static void pciDriverRestorePower()
{
    for(auto& saved : pciPowerCommands) pciConfigWriteWord(saved.first, PCI_CONFIG_OFF_COMMAND, saved.second);
    pciPowerCommands.clear();
    pciPowerCoordinator = G_TID_NONE;
}

/**
 * The PCI service owns bus-master control as well as firmware configuration
 * access. After quiescence only the authenticated kernel coordinator may issue
 * configuration requests, until cancellation restores the saved command words.
 */
static bool pciDriverHandlePower(const g_message_header* header, const uint8_t* content)
{
    if(header->sender != G_TID_NONE || header->length != sizeof(g_power_event)) return false;
    g_power_event event;
    memcpy(&event, content, sizeof(event));
    if(event.magic != G_POWER_EVENT_MAGIC) return true;
    if(event.type == G_POWER_EVENT_CANCEL)
    {
        if(event.request == pciPowerRequest)
        {
            pciDriverRestorePower();
            g_power_acknowledge(event.request, 1);
        }
        return true;
    }
    if(event.type != G_POWER_EVENT_PREPARE) return true;
    pciPowerRequest = event.request;
    pciPowerCoordinator = event.coordinator;
    bool ready = true;
    try
    {
        for(auto device = deviceList; device; device = device->next)
        {
            auto command = pciConfigReadWord(device, PCI_CONFIG_OFF_COMMAND);
            if(!(command & 4)) continue;
            pciPowerCommands.push_back({device, command});
            pciConfigWriteWord(device, PCI_CONFIG_OFF_COMMAND, command & ~4u);
            if(pciConfigReadWord(device, PCI_CONFIG_OFF_COMMAND) & 4) { ready = false; break; }
        }
    }
    catch(const std::bad_alloc&) { ready = false; }
    if(!ready) pciDriverRestorePower();
    g_power_acknowledge(event.request, ready);
    return true;
}

/**
 * Reply with the protocol's failure shape rather than abandoning a client that
 * happened to issue a request while the device configuration was quiesced.
 */
static void pciDriverRejectPowerRequest(const g_message_header* header, g_pci_command command)
{
    uint8_t response[sizeof(g_pci_read_bar_response)]{};
    size_t length = 0;
    switch(command)
    {
    case G_PCI_READ_CONFIG: length = sizeof(g_pci_read_config_response); break;
    case G_PCI_WRITE_CONFIG: length = sizeof(g_pci_write_config_response); break;
    case G_PCI_READ_BAR: length = sizeof(g_pci_read_bar_response); break;
    case G_PCI_ENABLE_RESOURCE_ACCESS: length = sizeof(g_pci_enable_resource_access_response); break;
    case G_PCI_LIST_DEVICES:
        length = sizeof(g_pci_list_devices_count_response);
        g_send_message_tm(header->sender, response, length, header->transaction, G_MESSAGE_SEND_MODE_NON_BLOCKING);
        break;
    default: return;
    }
    g_send_message_tm(header->sender, response, command == G_PCI_LIST_DEVICES ? 0 : length,
        header->transaction, G_MESSAGE_SEND_MODE_NON_BLOCKING);
}

int main()
{
	pciDriverScanBus();
	if(g_power_register(G_POWER_PLATFORM) != G_POWER_SUCCESSFUL) return -1;

	if(!g_task_register_name(G_PCI_DRIVER_NAME))
	{
		klog("PCI driver failed to register with identifier '%s'", G_PCI_DRIVER_NAME);
		return -1;
	}

	pciDriverReceiveMessages();
}

void pciDriverReceiveMessages()
{
	size_t buflen = sizeof(g_message_header) + 1024;
	uint8_t buf[buflen];
	for(;;)
	{
		auto result = g_receive_message(buf, buflen);
		if(result != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			klog("error receiving message, retrying");
			continue;
		}

		auto header = (g_message_header*) buf;
		if(pciDriverHandlePower(header, G_MESSAGE_CONTENT(buf))) continue;
		if(header->length < sizeof(g_pci_request_header)) continue;
		auto request = (g_pci_request_header*) G_MESSAGE_CONTENT(buf);
		if(pciPowerCoordinator != G_TID_NONE && header->sender != pciPowerCoordinator)
		{
			pciDriverRejectPowerRequest(header, request->command);
			continue;
		}

		if(request->command == G_PCI_LIST_DEVICES)
		{
			pciDriverHandleListDevices(header->sender, header->transaction);
		}
		else if(request->command == G_PCI_READ_CONFIG && header->length == sizeof(g_pci_read_config_request))
		{
			pciDriverHandleReadConfig(header->sender, header->transaction, (g_pci_read_config_request*) request);
		}
		else if(request->command == G_PCI_WRITE_CONFIG && header->length == sizeof(g_pci_write_config_request))
		{
			pciDriverHandleWriteConfig(header->sender, header->transaction, (g_pci_write_config_request*) request);
		}
		else if(request->command == G_PCI_ENABLE_RESOURCE_ACCESS &&
		        header->length == sizeof(g_pci_enable_resource_access_request))
		{
			pciDriverHandleEnableResourceAccess(header->sender, header->transaction,
			                                (g_pci_enable_resource_access_request*) request);
		}
		else if(request->command == G_PCI_READ_BAR)
		{
			pciDriverHandleReadBar(header->sender, header->transaction, (g_pci_read_bar_request*) request);
		}

	}
}

g_pci_device* _pciDriverGetDevice(g_pci_device_address deviceAddress)
{
	g_pci_device* device = nullptr;

	g_mutex_acquire(deviceListLock);
	g_pci_device* current = deviceList;
	while(current)
	{
		if(G_PCI_DEVICE_ADDRESS_BUILD(current->bus, current->device, current->function) == deviceAddress)
		{
			device = current;
			break;
		}

		current = current->next;
	}
	g_mutex_release(deviceListLock);

	return device;
}

void pciDriverHandleListDevices(g_tid sender, g_message_transaction transaction)
{
	g_mutex_acquire(deviceListLock);

	g_pci_list_devices_count_response response{};
	response.numDevices = deviceCount;
	g_send_message_t(sender, &response, sizeof(response), transaction);

	size_t dataSize = deviceCount * sizeof(g_pci_device_data);
	auto data = (g_pci_device_data*) malloc(dataSize);
	int pos = 0;
	g_pci_device* current = deviceList;
	while(current)
	{
		auto& entry = data[pos++];
		entry.deviceAddress = G_PCI_DEVICE_ADDRESS_BUILD(current->bus, current->device, current->function);
		entry.classCode = current->classCode;
		entry.subclassCode = current->subclassCode;
		entry.progIf = current->progIf;

		current = current->next;
	}

	g_mutex_release(deviceListLock);

	g_send_message_t(sender, data, dataSize, transaction);
	free(data);
}

void pciDriverHandleReadConfig(g_tid sender, g_message_transaction transaction, g_pci_read_config_request* request)
{
	g_pci_read_config_response response{};
	if((request->bytes != 1 && request->bytes != 2 && request->bytes != 4) || (request->offset & (request->bytes - 1)))
	{
		g_send_message_t(sender, &response, sizeof(response), transaction);
		return;
	}
	auto device = _pciDriverGetDevice(request->deviceAddress);
	if(!device)
	{
		response.successful = false;
		g_send_message_t(sender, &response, sizeof(response), transaction);
		return;
	}

	if(request->bytes == 1)
	{
		response.value = pciConfigReadByte(device, request->offset);
		response.successful = true;
	}
	else if(request->bytes == 2)
	{
		response.value = pciConfigReadWord(device, request->offset);
		response.successful = true;
	}
	else if(request->bytes == 4)
	{
		response.value = pciConfigReadDword(device, request->offset);
		response.successful = true;
	}
	else
	{
		klog("failed to read %i bytes from offset %i", request->bytes, request->offset);
		response.successful = false;
	}
	g_send_message_t(sender, &response, sizeof(response), transaction);
}

void pciDriverHandleWriteConfig(g_tid sender, g_message_transaction transaction, g_pci_write_config_request* request)
{
	g_pci_write_config_response response{};
	if((request->bytes != 1 && request->bytes != 2 && request->bytes != 4) || (request->offset & (request->bytes - 1)))
	{
		g_send_message_t(sender, &response, sizeof(response), transaction);
		return;
	}
	auto device = _pciDriverGetDevice(request->deviceAddress);
	if(!device)
	{
		response.successful = false;
		g_send_message_t(sender, &response, sizeof(response), transaction);
		return;
	}

	if(request->bytes == 1)
	{
		pciConfigWriteByte(device, request->offset, request->value);
		response.successful = true;
	}
	else if(request->bytes == 2)
	{
		pciConfigWriteWord(device, request->offset, request->value);
		response.successful = true;
	}
	else if(request->bytes == 4)
	{
		pciConfigWriteDword(device, request->offset, request->value);
		response.successful = true;
	}
	else
	{
		klog("failed to write %i bytes to offset %i", request->bytes, request->offset);
		response.successful = false;
	}
	g_send_message_t(sender, &response, sizeof(response), transaction);
}

void pciDriverHandleEnableResourceAccess(g_tid sender, g_message_transaction transaction,
                                     g_pci_enable_resource_access_request* request)
{
	g_pci_enable_resource_access_response response{};
	response.successful = false;

	auto device = _pciDriverGetDevice(request->deviceAddress);
	if(device && request->enabled <= 1)
	{
		pciEnableResourceAccess(device, request->enabled);
		response.successful = true;
	}

	g_send_message_t(sender, &response, sizeof(response), transaction);
}

void pciDriverHandleReadBar(g_tid sender, g_message_transaction transaction, g_pci_read_bar_request* request)
{
	g_pci_read_bar_response response{};
	response.successful = false;

	auto device = _pciDriverGetDevice(request->deviceAddress);
	if(device)
	{
		response.value = pciConfigGetBAR(device, request->bar);
		response.successful = true;
	}

	g_send_message_t(sender, &response, sizeof(response), transaction);
}

void pciDriverScanBus()
{
	int total = 0;
	for(uint16_t bus = 0; bus < 2 /* TODO: How much should we really scan? PCI_NUM_BUSES */; bus++)
	{
		for(uint8_t dev = 0; dev < PCI_NUM_DEVICES; dev++)
		{
			for(uint8_t fun = 0; fun < PCI_NUM_FUNCTIONS; fun++)
			{
				uint32_t header = pciConfigReadDwordAt(bus, dev, fun, PCI_CONFIG_OFF_CLASS);
				uint8_t classCode = (header >> 24) & 0xFF;
				uint8_t subClass = (header >> 16) & 0xFF;
				uint8_t progIf = (header >> 8) & 0xFF;

				if(classCode != 0xFF)
				{
					g_mutex_acquire(deviceListLock);
					auto device = new g_pci_device();
					device->bus = bus;
					device->device = dev;
					device->function = fun;
					device->classCode = classCode;
					device->subclassCode = subClass;
					device->progIf = progIf;
					device->next = deviceList;
					deviceList = device;
					deviceCount++;
					g_mutex_release(deviceListLock);

					++total;
				}
			}
		}
	}

	klog("PCI driver identified %i devices", total);
}

void pciEnableResourceAccess(g_pci_device* dev, bool enabled)
{
	uint16_t command = pciConfigReadWord(dev, PCI_CONFIG_OFF_COMMAND);
	const uint8_t flags = 0x0007;

	if(enabled)
		command |= flags;
	else
		command &= ~flags;

	pciConfigWriteWord(dev, PCI_CONFIG_OFF_COMMAND, command);
}

uint32_t pciConfigGetBAR(g_pci_device* dev, int bar)
{
	uint8_t offset = PCI_CONFIG_OFF_BAR0 + (0x4 * bar);

	uint32_t barVal = pciConfigReadDword(dev, offset);
	uint32_t mask = (barVal & PCI_CONFIG_BAR_IO) ? 0x3 : 0xF;
	return barVal & ~mask;
}

uint8_t pciConfigReadByte(g_pci_device* dev, uint8_t offset)
{
	g_mutex_acquire(configSpaceLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(dev->bus, dev->device, dev->function, offset));
	auto value = g_io_port_read_byte(PCI_CONFIG_PORT_DATA + (offset & 3));
	g_mutex_release(configSpaceLock);
	return value;
}

uint16_t pciConfigReadWord(g_pci_device* dev, uint8_t offset)
{
	g_mutex_acquire(configSpaceLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(dev->bus, dev->device, dev->function, offset));
	auto value = g_io_port_read_word(PCI_CONFIG_PORT_DATA + (offset & 2));
	g_mutex_release(configSpaceLock);
	return value;
}

uint32_t pciConfigReadDword(g_pci_device* dev, uint8_t offset)
{
	return pciConfigReadDwordAt(dev->bus, dev->device, dev->function, offset);
}

uint32_t pciConfigReadDwordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	g_mutex_acquire(configSpaceLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(bus, device, function, offset));
	auto result = g_io_port_read_dword(PCI_CONFIG_PORT_DATA);
	g_mutex_release(configSpaceLock);
	return result;
}

void pciConfigWriteByte(g_pci_device* dev, uint8_t offset, uint8_t value)
{
	g_mutex_acquire(configSpaceLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(dev->bus, dev->device, dev->function, offset));
	g_io_port_write_byte(PCI_CONFIG_PORT_DATA + (offset & 3), value);
	g_mutex_release(configSpaceLock);
}

void pciConfigWriteWord(g_pci_device* dev, uint8_t offset, uint16_t value)
{
	g_mutex_acquire(configSpaceLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(dev->bus, dev->device, dev->function, offset));
	g_io_port_write_word(PCI_CONFIG_PORT_DATA + (offset & 2), value);
	g_mutex_release(configSpaceLock);
}

void pciConfigWriteDword(g_pci_device* dev, uint8_t offset, uint32_t value)
{
	pciConfigWriteDwordAt(dev->bus, dev->device, dev->function, offset, value);
}

void pciConfigWriteDwordAt(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value)
{
	g_mutex_acquire(configSpaceLock);
	g_io_port_write_dword(PCI_CONFIG_PORT_ADDR, PCI_CONFIG_OFF(bus, device, function, offset));
	g_io_port_write_dword(PCI_CONFIG_PORT_DATA, value);
	g_mutex_release(configSpaceLock);
}
