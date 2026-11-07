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

#include "disk/storage.hpp"
#include <map>

struct g_volume_record
{
	g_volume_info info;
	bool active;
	bool manual;
	bool controlPending;
	uint64_t releasedClaim;
	g_tid releasedOwner;
};

struct g_volume_disk
{
	g_volume_disk_snapshot snapshot;
	g_volume_job_id job;
	g_volume_info info;
	g_tid owner;
	g_volume_id allowedVolume;
	uint64_t claim;
	uint64_t releasedClaim;
	g_tid releasedOwner;
};

/**
 * The message loop serializes registry access. A claim remains reserved until
 * its owner releases it or exits; losing a reply does not transfer ownership.
 */
struct g_volume_registry
{
	std::map<g_device_id, g_volume_disk> disks;
	std::map<g_volume_id, g_volume_record> volumes;
	g_volume_id nextVolume;
	uint64_t nextClaim;
};

/**
 * Returns the registered disk for device, or null if it is unknown.
 */
g_volume_disk* registryDiskFind(g_volume_registry* registry, g_device_id device);

/**
 * Returns the existing disk record or creates an empty record for device.
 */
g_volume_disk* registryDiskRegister(g_volume_registry* registry, g_device_id device);

/**
 * Treats missing disks as busy and checks for conflicting jobs or disk owners,
 * claimed volumes and pending volume controls.
 */
bool registryDiskIsBusy(g_volume_registry* registry, g_device_id device, g_tid requester, g_volume_job_id job);

/**
 * Clears the disk reservation and its allowed-volume exception.
 */
void registryDiskClearOwner(g_volume_disk* disk);

/**
 * Reserves an idle disk for sender and records an optional allowed volume.
 * Reuses an existing reservation owned by sender.
 */
g_volume_response registryDiskHandleClaim(g_volume_registry* registry, g_tid sender, const g_volume_claim_disk_request* request);

/**
 * Releases the sender's authenticated disk claim, accepting retries of the
 * most recently completed release.
 */
g_volume_response registryDiskHandleRelease(g_volume_registry* registry, g_tid sender, const g_volume_release_disk_request* request);

/**
 * Returns a volume record by ID, including inactive records, or null if unknown.
 */
g_volume_record* registryVolumeFind(g_volume_registry* registry, g_volume_id volume);

/**
 * Reconciles volume records with the disk snapshot and publishes discovery or
 * removal events. Manual mode requests probing and suppresses automatic mounts.
 */
g_volume_status registryVolumeReconcileDisk(g_volume_registry* registry, const g_volume_info* disk, bool manual);

/**
 * Checks whether an active claimed volume reserves path, ignoring the volume
 * identified by except.
 */
bool registryVolumeIsPathClaimed(g_volume_registry* registry, const char* path, g_volume_id except);

/**
 * Clears the mount owner, claim, mount ID and path, making the volume available.
 */
void registryVolumeClearOwner(g_volume_info* info);

/**
 * Returns the current information for an active volume, or a not-found status.
 */
g_volume_response registryVolumeHandleGet(g_volume_registry* registry, const g_volume_get_request* request);

/**
 * Returns the next active volume after the requested ID, optionally filtered
 * by device ID.
 */
g_volume_response registryVolumeHandleList(g_volume_registry* registry, const g_volume_list_request* request);

/**
 * Reserves a volume and mount path for the sender, reusing matching claims.
 */
g_volume_response registryVolumeHandleClaim(g_volume_registry* registry, g_tid sender, const g_volume_claim_request* request);

/**
 * Authenticates the mount claim and records the VFS mount ID, marking the
 * volume as mounted.
 */
g_volume_response registryVolumeHandleCommit(g_volume_registry* registry, g_tid sender, const g_volume_commit_request* request);

/**
 * Clears the sender's mount claim and makes the volume available, accepting
 * retries of its most recently completed release.
 */
g_volume_response registryVolumeHandleRelease(g_volume_registry* registry, g_tid sender, const g_volume_release_request* request);

/**
 * Records sender as the filesystem handler when the volume has neither an
 * owner nor a handler.
 */
g_volume_response registryVolumeHandleOffer(g_volume_registry* registry, g_tid sender, const g_volume_offer_request* request);

#endif
