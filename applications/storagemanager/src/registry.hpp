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

#ifndef STORAGEMANAGER_REGISTRY
#define STORAGEMANAGER_REGISTRY

#include <map>
#include <vector>
#include "storage.hpp"

struct g_volume_record
{
	g_volume_info info;
	bool active = true;
	bool manual = false;
	bool controlPending = false;
	uint64_t releasedClaim = 0;
	g_tid releasedOwner = G_TID_NONE;
};

struct g_volume_disk
{
	g_volume_disk_snapshot snapshot;
	g_volume_job_id job = 0;
	g_volume_info info;
	g_tid owner = G_TID_NONE;
	g_volume_id allowedVolume = 0;
	uint64_t claim = 0;
	uint64_t releasedClaim = 0;
	g_tid releasedOwner = G_TID_NONE;
};

/**
 * Access is serialized by the manager's message loop. Claims are reservations
 * until COMMIT, and have no expiry: a lost response cannot authorize a rival.
 */
class g_volume_registry
{
public:
	std::map<g_device_id, g_volume_disk> disks;
	std::map<g_volume_id, g_volume_record> volumes;
	g_volume_id nextVolume = 1;
	uint64_t nextClaim = 1;

	g_volume_response storageManagerHandleRequest(g_tid sender, const g_volume_request& request);
	void storageManagerForgetOwner(g_tid owner);

	bool diskBusy(g_device_id device, g_tid requester, g_volume_job_id job = 0) const;
	std::vector<g_volume_event> diskReplace(const g_volume_info& disk, const std::vector<g_volume_info>& discovered);

	bool volumePathClaimed(const char* path, g_volume_id except = 0) const;

private:
	g_volume_response diskHandleRequest(g_tid sender, const g_volume_request& request);
	g_volume_status volumeClaim(g_tid sender, const g_volume_request& request, g_volume_info& info);
};
#endif
