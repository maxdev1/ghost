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

#include "manager.hpp"
#include <ghost.h>
#include <libpci/driver.hpp>
#include <libdevice/interface.hpp>
#include <cstdio>
#include <unordered_map>

void _deviceManagerCheckPciDevices();
void _deviceManagerAwaitCommands();
void _deviceManagerHandleRegisterDevice(g_tid sender, g_message_transaction tx,
                                        g_device_manager_register_device_request* content);

static g_user_mutex devicesLock = g_mutex_initialize_r(true);
static std::unordered_map<g_device_id, device_t*> devices;
static g_device_id nextDeviceId = 1;

int main()
{
	g_tid comHandler = g_create_task((void*) _deviceManagerAwaitCommands);
	_deviceManagerCheckPciDevices();

	g_join(comHandler);
}

void _deviceManagerCheckPciDevices()
{
	int num;
	g_pci_device_data* devices;
	if(!pciDriverListDevices(&num, &devices))
	{
		klog("Failed to list PCI devices");
	}

	bool foundVmsvga = false;

	for(int i = 0; i < num; i++)
	{
		if(devices[i].classCode == PCI_BASE_CLASS_DISPLAY &&
		   devices[i].subclassCode == PCI_03_SUBCLASS_VGA &&
		   devices[i].progIf == PCI_03_00_PROGIF_VGA_COMPATIBLE)
		{
			uint32_t vendorId;
			if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_VENDOR_ID, 2, &vendorId))
			{
				klog("Failed to read vendor ID from PCI device %x", devices[i].deviceAddress);
				continue;
			}

			uint32_t deviceId;
			if(!pciDriverReadConfig(devices[i].deviceAddress, PCI_CONFIG_OFF_DEVICE_ID, 2, &deviceId))
			{
				klog("Failed to read device ID from PCI device %x", devices[i].deviceAddress);
				continue;
			}

			if(vendorId == 0x15AD /* VMWare */ && deviceId == 0x0405 /* SVGA2 */)
			{
				foundVmsvga = true;
			}
		}
	}
	pciDriverFreeDeviceList(devices);

	// TODO Implement something more sophisticated
	if(foundVmsvga)
	{
		klog("starting VMSVGA driver");
		g_spawn("/applications/vmsvgadriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
	else
	{
		klog("starting VBE driver");
		g_spawn("/applications/vbedriver.bin", "", "", G_SECURITY_LEVEL_DRIVER);
	}
}

void _deviceManagerAwaitCommands()
{
	if(!g_task_register_name(G_DEVICE_MANAGER_NAME))
	{
		klog("failed to register as %s", G_DEVICE_MANAGER_NAME);
		g_exit(-1);
	}

	size_t bufLen = 1024;
	uint8_t buf[bufLen];

	while(true)
	{
		auto status = g_receive_message(buf, bufLen);
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
			continue;
		auto message = (g_message_header*) buf;
		auto content = (g_device_manager_header*) G_MESSAGE_CONTENT(message);

		if(content->command == G_DEVICE_MANAGER_REGISTER_DEVICE)
		{
			_deviceManagerHandleRegisterDevice(message->sender, message->transaction,
			                                   (g_device_manager_register_device_request*) content);
		}
	}
}

void _deviceManagerHandleRegisterDevice(g_tid sender, g_message_transaction tx,
                                        g_device_manager_register_device_request* content)
{
	g_mutex_acquire(devicesLock);
	auto id = nextDeviceId++;
	auto device = new device_t();
	device->id = id;
	device->handler = content->handler;
	device->type = content->type;
	devices[id] = device;
	g_mutex_release(devicesLock);

	// Respond to registerer
	g_device_manager_register_device_response response{};
	response.status = G_DEVICE_MANAGER_SUCCESS;
	response.id = id;
	g_send_message_t(sender, &response, sizeof(response), tx);

	// Post to topic
	g_device_event_device_registered event{};
	event.header.event = G_DEVICE_EVENT_DEVICE_REGISTERED;
	event.id = device->id;
	event.type = content->type;
	event.driver = device->handler;
	g_send_topic_message(G_DEVICE_EVENT_TOPIC, &event, sizeof(event));
}
