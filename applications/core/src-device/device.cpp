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
#include <libdevice/manager.hpp>
#include <cstdio>
#include <cstring>

static const char* deviceTypeText(uint16_t type)
{
	switch(type)
	{
		case G_DEVICE_TYPE_VIDEO:
			return "video";
		case G_DEVICE_TYPE_BLOCK:
			return "block";
		case G_DEVICE_TYPE_NETWORK:
			return "network";
		default:
			return "unknown";
	}
}

static int deviceList()
{
	g_device_id afterId = 0;
	bool heading = false;
	while(true)
	{
		g_device_manager_list_devices_response response{};
		if(!deviceManagerListDevices(afterId, &response))
		{
			fprintf(stderr, "device: failed to list devices from device manager\n");
			return 1;
		}

		if(!heading)
		{
			printf("ID\tTYPE\tHANDLER\n");
			heading = true;
		}

		for(uint16_t i = 0; i < response.count; ++i)
		{
			const auto& device = response.devices[i];
			printf("%u\t%s (%u)\t%i\n", unsigned(device.id), deviceTypeText(device.type), unsigned(device.type),
				   device.handler);
			afterId = device.id;
		}

		if(response.count < G_DEVICE_MANAGER_LIST_MAX_DEVICES)
			break;
	}

	if(fflush(stdout) == EOF || ferror(stdout))
	{
		fprintf(stderr, "device: cannot write standard output\n");
		return 1;
	}

	return 0;
}

int main(int argc, char** argv)
{
	if(argc == 2 && coreArgumentIsHelp(argv[1]))
	{
		printf("Usage: device list\nList devices registered with device manager.\n");
		return 0;
	}

	if(argc == 2 && strcmp(argv[1], "list") == 0)
		return deviceList();

	fprintf(stderr, "Usage: device list\n");
	return 1;
}
