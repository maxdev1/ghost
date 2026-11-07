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

#ifndef LIBSTORAGE_INTERFACE
#define LIBSTORAGE_INTERFACE

#include <cstddef>
#include <libdevice/interface.hpp>

constexpr const char* G_STORAGE_MANAGER_NAME = "storagemanager";
constexpr const char* G_VOLUME_EVENT_TOPIC = "volume";
constexpr uint32_t G_VOLUME_MAGIC = 0x47564f4c;
constexpr uint32_t G_VOLUME_RESPONSE_MAGIC = 0x47565250;
constexpr uint16_t G_VOLUME_VERSION = 5;
constexpr uint64_t G_VOLUME_TIMEOUT_MS = 30000;

using g_volume_id = uint64_t;

enum g_volume_status : uint32_t
{
	/**
	 * The operation completed successfully.
	 */
	G_VOLUME_OK,

	/**
	 * The request or supplied partition layout is invalid.
	 */
	G_VOLUME_INVALID,

	/**
	 * No matching volume, disk, serving worker or next list entry exists.
	 */
	G_VOLUME_NOT_FOUND,

	/**
	 * An existing volume, disk or mount-path claim blocks this request.
	 */
	G_VOLUME_CONFLICT,

	/**
	 * The sender or claim token does not match the current ownership.
	 */
	G_VOLUME_NOT_OWNER,

	/**
	 * An active mount or pending operation prevents the requested action.
	 */
	G_VOLUME_BUSY,

	/**
	 * A storage or filesystem operation failed; the mount state may have changed.
	 */
	G_VOLUME_IO_ERROR,

	/**
	 * The disk geometry, partition layout or requested operation is unsupported.
	 */
	G_VOLUME_UNSUPPORTED,

	/**
	 * The deadline expired; accepted work and existing claims remain active.
	 */
	G_VOLUME_TIMED_OUT,

	/**
	 * The service, filesystem worker or IPC transport is unavailable.
	 */
	G_VOLUME_UNAVAILABLE,

	/**
	 * A response failed protocol validation.
	 */
	G_VOLUME_PROTOCOL_ERROR
};

enum g_volume_state : uint16_t
{
	/**
	 * No filesystem worker currently holds a mount claim.
	 */
	G_VOLUME_AVAILABLE,

	/**
	 * A worker holds the claim but has not confirmed its VFS mount.
	 */
	G_VOLUME_MOUNTING,

	/**
	 * The owner confirmed its VFS mount through COMMIT.
	 */
	G_VOLUME_MOUNTED
};

enum g_volume_command : uint16_t
{
	/**
	 * Client -> storage manager.
	 * Get the volume's current information and mount ownership.
	 */
	G_VOLUME_GET = 1,

	/**
	 * Client -> storage manager.
	 * Get the next volume after request.volume; device zero selects all disks.
	 */
	G_VOLUME_LIST,

	/**
	 * Filesystem worker -> storage manager.
	 * Reserve the volume and mount name. Matching retries reuse the claim token.
	 */
	G_VOLUME_CLAIM,

	/**
	 * Filesystem worker -> storage manager.
	 * Report the VFS mount ID created under this worker's claim.
	 */
	G_VOLUME_COMMIT,

	/**
	 * Filesystem worker -> storage manager.
	 * Give up the reservation after unmounting or a failed mount attempt.
	 */
	G_VOLUME_RELEASE,

	/**
	 * Client -> storage manager -> selected filesystem worker.
	 * Mount the volume, optionally at /mount/request.name.
	 */
	G_VOLUME_MOUNT,

	/**
	 * Client -> storage manager -> owning filesystem worker.
	 * Flush and unmount the volume. A nonzero request.claim must match its claim.
	 */
	G_VOLUME_UNMOUNT,

	/**
	 * Client -> storage manager.
	 * Rescan an unmounted disk, optionally under the client's disk reservation.
	 */
	G_VOLUME_RESCAN,

	/**
	 * Client -> storage manager.
	 * Reserve the disk, optionally allowing request.volume to mount for this client.
	 */
	G_VOLUME_CLAIM_DISK,

	/**
	 * Client -> storage manager.
	 * Give up this client's disk reservation after changes and rescanning.
	 */
	G_VOLUME_RELEASE_DISK,

	/**
	 * Client -> storage manager -> selected filesystem worker.
	 * Flush pending filesystem data while keeping the volume mounted.
	 */
	G_VOLUME_FLUSH,

	/**
	 * Filesystem worker -> storage manager.
	 * Report that this worker recognizes the volume and can mount it later.
	 */
	G_VOLUME_OFFER,

	/**
	 * Client -> storage manager -> selected filesystem worker.
	 * Get filesystem properties. Intermediate replies report scan progress.
	 */
	G_VOLUME_STAT
};

enum g_volume_scheme : uint16_t
{
	/**
	 * The volume spans an unpartitioned disk.
	 */
	G_VOLUME_WHOLE_DISK,

	/**
	 * The volume is a primary or logical MBR partition.
	 */
	G_VOLUME_MBR,

	/**
	 * The volume is a GPT partition.
	 */
	G_VOLUME_GPT
};

struct g_volume_info
{
	g_volume_id id = 0;
	g_device_id device = 0;

	g_volume_scheme scheme = G_VOLUME_WHOLE_DISK;

	g_tid driver = G_TID_NONE;
	uint64_t firstSector = 0;
	uint64_t sectorCount = 0;
	uint32_t sectorSize = 0;
	uint32_t partition = 0;

	g_volume_state state = G_VOLUME_AVAILABLE;

	g_tid owner = G_TID_NONE;
	g_tid handler = G_TID_NONE;
	uint64_t claim = 0;

	int32_t mount = -1;
	char filesystem[16]{};
	char path[64]{};
};

struct g_volume_header
{
	uint32_t magic = G_VOLUME_MAGIC;
	uint16_t version = G_VOLUME_VERSION;
	g_volume_command command = G_VOLUME_GET;
};

struct g_volume_get_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_GET;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
};

struct g_volume_list_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_LIST;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
	g_device_id device = 0;
};

struct g_volume_claim_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_CLAIM;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
	char filesystem[16]{};
	char name[32]{};
};

struct g_volume_commit_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_COMMIT;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
	uint64_t claim = 0;
	int32_t mount = -1;
};

struct g_volume_release_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_RELEASE;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
	uint64_t claim = 0;
};

struct g_volume_mount_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_MOUNT;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
	char name[32]{};
};

struct g_volume_unmount_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_UNMOUNT;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
	uint64_t claim = 0;
};

struct g_volume_rescan_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_RESCAN;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_device_id device = 0;
};

struct g_volume_claim_disk_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_CLAIM_DISK;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_device_id device = 0;
	g_volume_id volume = 0;
};

struct g_volume_release_disk_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_RELEASE_DISK;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_device_id device = 0;
	uint64_t claim = 0;
};

struct g_volume_flush_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_FLUSH;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
};

struct g_volume_offer_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_OFFER;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
	char filesystem[16]{};
};

struct g_volume_stat_request
{
	static constexpr g_volume_command COMMAND = G_VOLUME_STAT;
	g_volume_header header{G_VOLUME_MAGIC, G_VOLUME_VERSION, COMMAND};
	g_volume_id volume = 0;
};

constexpr size_t storageManagerRequestSize(g_volume_command command)
{
	switch(command)
	{
		case G_VOLUME_GET: return sizeof(g_volume_get_request);
		case G_VOLUME_LIST: return sizeof(g_volume_list_request);
		case G_VOLUME_CLAIM: return sizeof(g_volume_claim_request);
		case G_VOLUME_COMMIT: return sizeof(g_volume_commit_request);
		case G_VOLUME_RELEASE: return sizeof(g_volume_release_request);
		case G_VOLUME_MOUNT: return sizeof(g_volume_mount_request);
		case G_VOLUME_UNMOUNT: return sizeof(g_volume_unmount_request);
		case G_VOLUME_RESCAN: return sizeof(g_volume_rescan_request);
		case G_VOLUME_CLAIM_DISK: return sizeof(g_volume_claim_disk_request);
		case G_VOLUME_RELEASE_DISK: return sizeof(g_volume_release_disk_request);
		case G_VOLUME_FLUSH: return sizeof(g_volume_flush_request);
		case G_VOLUME_OFFER: return sizeof(g_volume_offer_request);
		case G_VOLUME_STAT: return sizeof(g_volume_stat_request);
		default: return 0;
	}
}

struct g_volume_filesystem_info
{
	char type[16]{};
	uint32_t allocationUnit = 0;
	uint64_t totalBytes = 0;
	uint64_t freeBytes = 0;
	uint8_t writable = 0;
};

struct g_volume_response
{
	uint32_t magic = G_VOLUME_RESPONSE_MAGIC;
	uint16_t version = G_VOLUME_VERSION;
	g_volume_command command = G_VOLUME_GET;
	g_volume_status status = G_VOLUME_INVALID;
	g_volume_info info;
	g_volume_filesystem_info filesystem;
	/**
	 * A nonzero total identifies an intermediate STAT reply, measured in allocation
	 * units. Final replies leave both counters zero.
	 */
	uint64_t completed = 0;
	uint64_t total = 0;
};

enum g_volume_event_type : uint16_t
{
	/**
	 * A disk was discovered or rescanned; info.id is zero.
	 */
	G_VOLUME_DISK_DISCOVERED,

	/**
	 * A volume is available for probing; rescans may repeat the announcement.
	 */
	G_VOLUME_DISCOVERED,

	/**
	 * A rescan retired this volume ID because its storage region changed or disappeared.
	 */
	G_VOLUME_REMOVED,

	/**
	 * Reprobe an administrative change without automatically mounting the volume.
	 */
	G_VOLUME_PROBE
};

struct g_volume_event
{
	uint32_t magic = G_VOLUME_MAGIC;
	uint16_t version = G_VOLUME_VERSION;
	g_volume_event_type type = G_VOLUME_DISCOVERED;
	g_volume_info info;
};

#endif
