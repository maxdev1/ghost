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

#include <ghost.h>
#include "install.hpp"

std::vector<g_install_disk> installDiscoverDisks()
{
	std::vector<g_install_disk> disks;
	auto started = g_millis();
	do
	{
		disks.clear();
		g_device_id after = 0;
		g_disk_info info;
		while(diskNext(after, &info) == G_VOLUME_OK)
		{
			disks.push_back({info.device, info});
			after = info.device;
		}
		if(!disks.empty())
			break;
		g_sleep(50);
	} while(g_millis() - started < 10000);
	return disks;
}

std::vector<g_install_destination> installDiscoverDestinations()
{
	std::vector<g_install_destination> result;
	for(const auto& disk: installDiscoverDisks())
	{
		result.push_back({disk, {}});
		g_volume_info volume;
		g_volume_id after = 0;
		while(volumeNext(after, disk.id, &volume) == G_VOLUME_OK)
		{
			result.push_back({disk, volume});
			after = volume.id;
		}
	}
	return result;
}
