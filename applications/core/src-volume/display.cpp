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

#include "volume.hpp"

const char* volumeToolStatusText(g_volume_status status)
{
	switch(status)
	{
		case G_VOLUME_INVALID:
			return "invalid request";
		case G_VOLUME_NOT_FOUND:
			return "volume or filesystem driver not found";
		case G_VOLUME_CONFLICT:
			return "mount name or ownership conflicts with the request";
		case G_VOLUME_NOT_OWNER:
			return "mount ownership changed";
		case G_VOLUME_BUSY:
			return "volume is busy or protected";
		case G_VOLUME_IO_ERROR:
			return "storage or filesystem I/O error";
		case G_VOLUME_UNSUPPORTED:
			return "unsupported operation";
		case G_VOLUME_TIMED_OUT:
			return "request timed out; its outcome is uncertain; run 'volume list' to check";
		case G_VOLUME_UNAVAILABLE:
			return "storage manager or filesystem worker unavailable";
		case G_VOLUME_PROTOCOL_ERROR:
			return "invalid manager or driver response";
		default:
			return "unknown error";
	}
}

const char* volumeToolStateText(g_volume_state state)
{
	switch(state)
	{
		case G_VOLUME_AVAILABLE:
			return "available";
		case G_VOLUME_MOUNTING:
			return "mounting";
		case G_VOLUME_MOUNTED:
			return "mounted";
		default:
			return "unknown";
	}
}

int volumeToolList()
{
	g_volume_id after = 0;
	bool heading = false;
	for(;;)
	{
		g_volume_info info;
		auto status = volumeNext(after, 0, &info);
		if(status == G_VOLUME_NOT_FOUND)
		{
			if(!heading)
				printf("No volumes found.\n");
			return 0;
		}

		if(status != G_VOLUME_OK)
		{
			fprintf(stderr, "volume: cannot list volumes: %s\n", volumeToolStatusText(status));
			return 1;
		}

		if(!heading)
		{
			printf("ID\tDISK\tFILESYSTEM\tSTATE\tMOUNT\n");
			heading = true;
		}

		printf("%llu\t%u\t%s\t%s\t%s\n", static_cast<unsigned long long>(info.id), unsigned(info.device),
			   info.filesystem[0] ? info.filesystem : "unknown", volumeToolStateText(info.state),
			   info.path[0] ? info.path : "-");
		after = info.id;
	}
}


void volumeToolPrintInfo(const g_volume_info& info)
{
	printf("Volume: %llu\nDisk: %u\nPartition: %u\nFirst sector: %llu\nSectors: %llu\n"
		   "Sector size: %u\nFilesystem: %s\nState: %s\nMount: %s\n",
		   (unsigned long long) info.id, unsigned(info.device), info.partition, (unsigned long long) info.firstSector,
		   (unsigned long long) info.sectorCount, info.sectorSize, info.filesystem[0] ? info.filesystem : "unknown",
		   volumeToolStateText(info.state), info.path[0] ? info.path : "-");
}
