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

#include "owners.hpp"
#include <ghost/filesystem.h>
#include <ghost/tasks.h>
#include <string.h>

static bool registryOwnerIsMountDetached(const g_volume_info* info)
{
	g_fs_stat_data stat = {};
	g_fs_stat_status status = g_fs_stat(info->path, &stat);
	if(status == G_FS_STAT_NOT_FOUND)
		return true;
	if(status == G_FS_STAT_SUCCESS)
		return info->mount >= 0 && stat.virtual_id != info->mount;

	if(strncmp(info->path, "/mount/", 7) || !info->path[7])
		return false;

	g_fs_directory_iterator* directory = g_open_directory_s("/mount", nullptr);
	if(!directory)
		return false;

	g_fs_read_directory_status readStatus;
	while(g_fs_directory_entry* entry = g_read_directory_s(directory, &readStatus))
	{
		if(strcmp(entry->name, info->path + 7))
			continue;

		bool detached = info->mount >= 0 && entry->node_id != info->mount;
		g_close_directory(directory);
		return detached;
	}

	bool detached = readStatus == G_FS_READ_DIRECTORY_EOD;
	g_close_directory(directory);
	return detached;
}

static bool registryOwnerIsDetached(g_volume_registry* registry, g_tid owner)
{
	for(auto& entry : registry->volumes)
		if(entry.second.info.owner == owner && !registryOwnerIsMountDetached(&entry.second.info))
			return false;
	return true;
}

void registryOwnerReap(g_volume_registry* registry)
{
	for(auto& entry : registry->volumes)
	{
		g_volume_record* volume = &entry.second;
		g_tid owner = volume->info.owner;
		if(owner == G_TID_NONE)
			owner = volume->info.handler;
		if(owner != G_TID_NONE && g_get_pid_for_tid(owner) == G_PID_NONE && registryOwnerIsDetached(registry, owner))
			registryOwnerForget(registry, owner);
	}
	for(auto& entry : registry->disks)
	{
		g_volume_disk* disk = &entry.second;
		if(disk->owner != G_TID_NONE && g_get_pid_for_tid(disk->owner) == G_PID_NONE &&
		   registryOwnerIsDetached(registry, disk->owner))
			registryOwnerForget(registry, disk->owner);
	}
}

void registryOwnerForget(g_volume_registry* registry, g_tid owner)
{
	for(auto& entry : registry->volumes)
	{
		g_volume_record* volume = &entry.second;
		if(volume->info.handler == owner)
			volume->info.handler = G_TID_NONE;
		if(volume->info.owner == owner)
			registryVolumeClearOwner(&volume->info);
	}
	for(auto& entry : registry->disks)
		if(entry.second.owner == owner)
			registryDiskClearOwner(&entry.second);
}
