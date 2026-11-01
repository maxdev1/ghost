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

#include "../src-common/arguments.hpp"
#include <ghost/messages.h>
#include <ghost/tasks.h>
#include <libblockdevice/driver.hpp>
#include <cstdio>
#include <cstring>
#include <vector>

struct g_blockdevice_entry
{
	g_device_id id;
	g_tid driver;
};

/**
 * Replays retained registrations from the device manager to locate each block
 * driver's IPC endpoint. Duplicate announcements do not add duplicate devices.
 */
static bool blockDeviceDiscover(std::vector<g_blockdevice_entry>& devices)
{
	auto manager = g_task_get_by_name(G_DEVICE_MANAGER_NAME);
	if(manager == G_TID_NONE)
	{
		fprintf(stderr, "blockdevice: device manager is unavailable\n");
		return false;
	}

	uint8_t incoming[sizeof(g_message_header) + G_MESSAGE_MAXIMUM_MESSAGE_LENGTH];
	auto cursor = G_MESSAGE_TOPIC_TRANSACTION_START;
	for(;;)
	{
		auto status = g_receive_topic_message_m(G_DEVICE_EVENT_TOPIC, incoming, sizeof(incoming), cursor,
												G_MESSAGE_RECEIVE_MODE_NON_BLOCKING);
		if(status == G_MESSAGE_RECEIVE_STATUS_EMPTY)
			return true;
		if(status != G_MESSAGE_RECEIVE_STATUS_SUCCESSFUL)
		{
			fprintf(stderr, "blockdevice: cannot discover devices (status %d)\n", status);
			return false;
		}

		auto message = reinterpret_cast<g_message_header*>(incoming);
		cursor = message->transaction;
		if(message->sender != manager || message->length != sizeof(g_device_event_device_registered))
			continue;
		g_device_event_device_registered event;
		memcpy(&event, G_MESSAGE_CONTENT(incoming), sizeof(event));
		if(event.header.event != G_DEVICE_EVENT_DEVICE_REGISTERED || event.type != G_DEVICE_TYPE_BLOCK || !event.id ||
		   event.driver == G_TID_NONE)
			continue;

		bool duplicate = false;
		for(const auto& device: devices)
			duplicate |= device.id == event.id;
		if(!duplicate)
			devices.push_back({event.id, event.driver});
	}
}

static const char* blockDeviceStatusText(g_block_status status)
{
	switch(status)
	{
		case G_BLOCK_INVALID_REQUEST:
			return "invalid request";
		case G_BLOCK_DEVICE_NOT_FOUND:
			return "device not found";
		case G_BLOCK_UNSUPPORTED:
			return "unsupported operation";
		case G_BLOCK_IO_ERROR:
			return "I/O error";
		case G_BLOCK_TIMEOUT:
			return "timeout";
		case G_BLOCK_UNAVAILABLE:
			return "driver or device unavailable";
		case G_BLOCK_PROTOCOL_ERROR:
			return "invalid driver response";
		case G_BLOCK_BUSY:
			return "device busy";
		default:
			return "unknown error";
	}
}

static bool blockDeviceQuery(const g_blockdevice_entry& device, g_block_device_info& info)
{
	auto status = blockDeviceGetInfo(device.driver, device.id, &info);
	if(status == G_BLOCK_SUCCESS)
		return true;
	fprintf(stderr, "blockdevice: cannot query device %u: %s (status %u)\n", unsigned(device.id),
			blockDeviceStatusText(status), unsigned(status));
	return false;
}

static uint64_t blockDeviceCapacityMiB(const g_block_device_info& info)
{
	return info.sectorCount / (1024 * 1024 / info.sectorSize);
}

static int blockDeviceList(const std::vector<g_blockdevice_entry>& devices)
{
	if(devices.empty())
	{
		printf("No block devices found.\n");
		return 0;
	}
	printf("ID\tMODEL\tSIZE (MiB)\n");
	int result = 0;
	for(const auto& device: devices)
	{
		g_block_device_info info{};
		if(blockDeviceQuery(device, info))
			printf("%u\t%s\t%llu\n", unsigned(device.id), info.model,
				   static_cast<unsigned long long>(blockDeviceCapacityMiB(info)));
		else
		{
			printf("%u\t(unavailable)\t-\n", unsigned(device.id));
			result = 1;
		}
	}
	return result;
}

static int blockDevicePrintInfo(const g_blockdevice_entry& device)
{
	g_block_device_info info{};
	if(!blockDeviceQuery(device, info))
		return 1;
	printf("ID: %u\nDriver task: %d\nModel: %s\nSerial: %s\n"
		   "Capacity: %llu MiB\nSectors: %llu\nSector size: %u bytes\n"
		   "Maximum transfer: %u sectors\nWritable: %s\nFlush supported: %s\n",
		   unsigned(device.id), int(device.driver), info.model, info.serial,
		   static_cast<unsigned long long>(blockDeviceCapacityMiB(info)),
		   static_cast<unsigned long long>(info.sectorCount), unsigned(info.sectorSize),
		   unsigned(info.maxTransferSectors), info.flags & G_BLOCK_WRITABLE ? "yes" : "no",
		   info.flags & G_BLOCK_FLUSH_SUPPORTED ? "yes" : "no");
	return 0;
}

static bool blockDeviceParseId(const char* text, g_device_id& id)
{
	uint64_t value;
	if(!coreParseUnsigned(text, &value, UINT16_MAX) || !value)
		return false;
	id = value;
	return true;
}

int main(int argc, char** argv)
{
	const char* usage = "Usage: blockdevice list\n       blockdevice ID get\n";
	if(argc == 2 && coreArgumentIsHelp(argv[1]))
	{
		printf("%sList block devices or query a device's driver. IDs are decimal.\n", usage);
		return 0;
	}
	bool list = argc == 2 && strcmp(argv[1], "list") == 0;
	bool get = argc == 3 && strcmp(argv[2], "get") == 0;
	if(!list && !get)
	{
		fprintf(stderr, "%s", usage);
		return 1;
	}
	g_device_id id = 0;
	if(get && !blockDeviceParseId(argv[1], id))
	{
		fprintf(stderr, "blockdevice: invalid device ID '%s'; expected 1 to 65535\n", argv[1]);
		return 1;
	}

	std::vector<g_blockdevice_entry> devices;
	if(!blockDeviceDiscover(devices))
		return 1;
	if(list)
		return blockDeviceList(devices);
	for(const auto& device: devices)
		if(device.id == id)
			return blockDevicePrintInfo(device);
	fprintf(stderr, "blockdevice: device %u not found\n", unsigned(id));
	return 1;
}
