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

#ifndef LIBSTORAGE_ADMIN
#define LIBSTORAGE_ADMIN
#include <cstddef>
#include <type_traits>
#include "volume.hpp"

constexpr uint32_t G_VOLUME_ADMIN_MAGIC = 0x47564144;
constexpr uint16_t G_VOLUME_ADMIN_VERSION = 4;
using g_volume_job_id = uint64_t;

enum g_volume_admin_command : uint16_t
{
	/**
	 * Return a registered disk's geometry and partition-table state.
	 */
	G_DISK_GET = 1,
	/**
	 * Return the next disk after request.device.
	 */
	G_DISK_NEXT,
	/**
	 * Return the next partition number after request.partition on request.device.
	 */
	G_DISK_PARTITION_NEXT,
	/**
	 * Initialize MBR after checking bounded metadata areas; GPT creation is unsupported.
	 */
	G_DISK_TABLE_CREATE,
	/**
	 * Allocate a primary MBR entry within a free, aligned region.
	 */
	G_DISK_PARTITION_CREATE,
	/**
	 * Remove a primary MBR entry without erasing its sector contents.
	 */
	G_DISK_PARTITION_DELETE,
	/**
	 * Zero every sector of the selected disk, including its partition tables.
	 */
	G_DISK_WIPE,
	/**
	 * Create a filesystem inside an unmounted volume without editing its table.
	 */
	G_VOLUME_FORMAT,
	/**
	 * Zero the selected volume's sector range, preserving its partition entry.
	 */
	G_VOLUME_WIPE,
	/**
	 * Return an administrative job's progress and final result.
	 */
	G_VOLUME_JOB_GET,
	/**
	 * Register the sender as a formatter for request.filesystem.
	 */
	G_VOLUME_FORMATTER_REGISTER,
	/**
	 * Manager-to-formatter request for a bounded filesystem initialization job.
	 */
	G_VOLUME_FORMAT_WORK,
	/**
	 * Acknowledge formatter completion so its authenticated worker can exit.
	 */
	G_VOLUME_FORMAT_ACK,
	/**
	 * Query a primary MBR volume's identity and boot space under a disk claim.
	 */
	G_DISK_BOOT_GET,
	/**
	 * Write, flush and verify a chunk before the first partition, excluding sector zero.
	 */
	G_DISK_BOOT_WRITE,
	/**
	 * Replace MBR boot code and identity while preserving partition geometry.
	 */
	G_DISK_BOOT_COMMIT,
};

enum g_disk_layout : uint32_t
{
	/**
	 * No partition table was recognized; the disk can contain a whole-disk filesystem.
	 */
	G_DISK_NONE,
	/**
	 * A valid MBR table, including an empty table.
	 */
	G_DISK_MBR,
	/**
	 * A valid GPT layout; editing is currently unsupported.
	 */
	G_DISK_GPT,
	/**
	 * Partition metadata is malformed or unreadable and must not be treated as empty.
	 */
	G_DISK_INVALID,
	/**
	 * The table or sector geometry is recognized but unsupported.
	 */
	G_DISK_UNSUPPORTED
};

enum g_volume_job_state : uint32_t
{
	/**
	 * The operation is accepted and may still be performing I/O or rescanning.
	 */
	G_VOLUME_JOB_RUNNING,
	/**
	 * The operation and required rescan completed successfully.
	 */
	G_VOLUME_JOB_SUCCEEDED,
	/**
	 * The operation failed; partial disk changes are possible.
	 */
	G_VOLUME_JOB_FAILED
};

enum g_volume_job_phase : uint32_t
{
	/**
	 * Validate the target and its existing metadata before writing.
	 */
	G_VOLUME_JOB_CHECKING,
	/**
	 * Zero-fill the selected disk or volume range.
	 */
	G_VOLUME_JOB_ZEROING,
	/**
	 * Write and verify partition-table metadata.
	 */
	G_VOLUME_JOB_EDITING,
	/**
	 * Initialize filesystem metadata through its registered formatter.
	 */
	G_VOLUME_JOB_FORMATTING,
	/**
	 * Refresh volume identities and filesystem recognition after writes.
	 */
	G_VOLUME_JOB_RESCANNING
};

constexpr uint32_t G_DISK_PARTITION_HAS_START = 1;
constexpr uint32_t G_DISK_PARTITION_REMAINING = 2;
/**
 * Explicitly replace partition metadata before creating an empty MBR.
 */
constexpr uint32_t G_DISK_TABLE_REPLACE = 1;
constexpr size_t G_DISK_BOOT_CODE_SIZE = 440;
constexpr size_t G_DISK_BOOT_CHUNK_SIZE = 1024;
constexpr uint32_t G_DISK_BOOT_ACTIVATE = 1;

struct g_disk_boot_info
{
	g_device_id device = 0;
	g_volume_id volume = 0;
	uint64_t claim = 0;
	uint64_t generation = 0;
	uint32_t diskIdentity = 0;
	uint32_t partition = 0;
	uint64_t firstSector = 0;
	uint64_t sectorCount = 0;
	/**
	 * Requested boot area in sectors, including sector zero.
	 */
	uint64_t bootSectors = 0;
};

struct g_disk_info
{
	g_device_id device = 0;
	g_tid driver = G_TID_NONE;
	uint64_t sectorCount = 0;
	uint32_t sectorSize = 0;
	uint32_t flags = 0;
	g_disk_layout layout = G_DISK_NONE;
	uint32_t partitions = 0;
	uint64_t firstUsable = 1;
	uint64_t lastUsable = 0;
	uint64_t generation = 0;
	g_volume_job_id activeJob = 0;
	uint8_t wholeFilesystem = 0;
	uint8_t extended = 0;
	char model[41]{};
	char serial[21]{};
};

struct g_disk_partition_info
{
	g_device_id device = 0;
	uint32_t number = 0;
	g_volume_id volume = 0;
	uint64_t firstSector = 0;
	uint64_t sectorCount = 0;
	g_volume_scheme scheme = G_VOLUME_MBR;
	uint32_t type = 0;
	uint8_t bootable = 0;
	uint8_t container = 0;
	uint8_t typeGuid[16]{};
};

struct g_volume_job_info
{
	g_volume_job_id id = 0;
	g_volume_admin_command command = G_DISK_GET;
	g_device_id device = 0;
	g_volume_id volume = 0;
	g_volume_job_state state = G_VOLUME_JOB_RUNNING;
	g_volume_job_phase phase = G_VOLUME_JOB_CHECKING;
	g_volume_status result = G_VOLUME_OK;
	uint64_t completed = 0;
	uint64_t total = 0;
	uint8_t ioSafe = 1;
	uint32_t partition = 0;
};

struct g_volume_admin_header
{
	uint32_t magic = G_VOLUME_ADMIN_MAGIC;
	uint16_t version = G_VOLUME_ADMIN_VERSION;
	g_volume_admin_command command = G_DISK_GET;
};

struct g_disk_get_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_GET;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	g_device_id device = 0;
};

struct g_disk_next_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_NEXT;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	g_device_id device = 0;
};

struct g_disk_partition_next_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_PARTITION_NEXT;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	g_device_id device = 0;
	uint32_t partition = 0;
};

struct g_disk_table_create_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_TABLE_CREATE;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	uint64_t operation = 0;
	uint64_t generation = 0;
	g_device_id device = 0;
	g_disk_layout layout = G_DISK_MBR;
	uint32_t flags = 0;
};

struct g_disk_partition_create_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_PARTITION_CREATE;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	uint64_t operation = 0;
	uint64_t generation = 0;
	g_device_id device = 0;
	uint64_t firstSector = 0;
	uint64_t sectorCount = 0;
	uint64_t alignment = 2048;
	uint32_t flags = 0;
	uint32_t type = 0x0c;
	uint32_t slot = 0;
};

struct g_disk_partition_delete_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_PARTITION_DELETE;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	uint64_t operation = 0;
	uint64_t generation = 0;
	g_device_id device = 0;
	uint32_t partition = 0;
};

struct g_disk_wipe_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_WIPE;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	uint64_t operation = 0;
	uint64_t generation = 0;
	g_device_id device = 0;
};

struct g_volume_format_request
{
	static constexpr g_volume_admin_command COMMAND = G_VOLUME_FORMAT;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	uint64_t operation = 0;
	uint64_t generation = 0;
	g_volume_id volume = 0;
	char filesystem[16]{};
	char label[12]{};
};

struct g_volume_wipe_request
{
	static constexpr g_volume_admin_command COMMAND = G_VOLUME_WIPE;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	uint64_t operation = 0;
	uint64_t generation = 0;
	g_volume_id volume = 0;
};

struct g_volume_job_get_request
{
	static constexpr g_volume_admin_command COMMAND = G_VOLUME_JOB_GET;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	g_volume_job_id job = 0;
};

struct g_volume_formatter_register_request
{
	static constexpr g_volume_admin_command COMMAND = G_VOLUME_FORMATTER_REGISTER;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	char filesystem[16]{};
};

struct g_volume_format_work_request
{
	static constexpr g_volume_admin_command COMMAND = G_VOLUME_FORMAT_WORK;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	g_volume_job_id job = 0;
	g_device_id device = 0;
	g_tid driver = G_TID_NONE;
	uint64_t firstSector = 0;
	uint64_t sectorCount = 0;
	char filesystem[16]{};
	char label[12]{};
};

struct g_volume_format_ack_request
{
	static constexpr g_volume_admin_command COMMAND = G_VOLUME_FORMAT_ACK;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	g_volume_job_id job = 0;
};

struct g_disk_boot_get_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_BOOT_GET;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	g_device_id device = 0;
	g_volume_id volume = 0;
	uint64_t claim = 0;
	uint64_t sectorCount = 0;
	uint32_t diskIdentity = 0;
};

struct g_disk_boot_write_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_BOOT_WRITE;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	uint64_t operation = 0;
	g_disk_boot_info boot;
	uint64_t firstSector = 0;
	uint32_t dataSize = 0;
	uint8_t data[G_DISK_BOOT_CHUNK_SIZE]{};
};

struct g_disk_boot_commit_request
{
	static constexpr g_volume_admin_command COMMAND = G_DISK_BOOT_COMMIT;
	g_volume_admin_header header{G_VOLUME_ADMIN_MAGIC, G_VOLUME_ADMIN_VERSION, COMMAND};
	uint64_t operation = 0;
	g_disk_boot_info boot;
	uint32_t flags = 0;
	uint8_t code[G_DISK_BOOT_CODE_SIZE]{};
};

constexpr size_t storageManagerAdminRequestSize(g_volume_admin_command command)
{
	switch(command)
	{
		case G_DISK_GET: return sizeof(g_disk_get_request);
		case G_DISK_NEXT: return sizeof(g_disk_next_request);
		case G_DISK_PARTITION_NEXT: return sizeof(g_disk_partition_next_request);
		case G_DISK_TABLE_CREATE: return sizeof(g_disk_table_create_request);
		case G_DISK_PARTITION_CREATE: return sizeof(g_disk_partition_create_request);
		case G_DISK_PARTITION_DELETE: return sizeof(g_disk_partition_delete_request);
		case G_DISK_WIPE: return sizeof(g_disk_wipe_request);
		case G_VOLUME_FORMAT: return sizeof(g_volume_format_request);
		case G_VOLUME_WIPE: return sizeof(g_volume_wipe_request);
		case G_VOLUME_JOB_GET: return sizeof(g_volume_job_get_request);
		case G_VOLUME_FORMATTER_REGISTER: return sizeof(g_volume_formatter_register_request);
		case G_VOLUME_FORMAT_WORK: return sizeof(g_volume_format_work_request);
		case G_VOLUME_FORMAT_ACK: return sizeof(g_volume_format_ack_request);
		case G_DISK_BOOT_GET: return sizeof(g_disk_boot_get_request);
		case G_DISK_BOOT_WRITE: return sizeof(g_disk_boot_write_request);
		case G_DISK_BOOT_COMMIT: return sizeof(g_disk_boot_commit_request);
		default: return 0;
	}
}

struct g_volume_admin_response
{
	uint32_t magic = G_VOLUME_ADMIN_MAGIC;
	uint16_t version = G_VOLUME_ADMIN_VERSION;
	g_volume_admin_command command = G_DISK_GET;
	g_volume_status status = G_VOLUME_INVALID;
	g_disk_info disk;
	g_disk_partition_info partition;
	g_volume_job_info job;
	g_volume_info volume;
	g_disk_boot_info boot;
};

/**
 * Administrative calls authenticate replies and never resend an accepted request.
 * Start a mutation with a stable operation ID, then poll the returned job ID.
 * Client timeout does not cancel I/O or release the target's administrative lock.
 */
g_volume_status storageManagerAdminRequest(const void* request, size_t size, g_volume_admin_response* response);
g_volume_status storageManagerAdminStart(const void* request, size_t size, g_volume_job_info* info);

template<typename Request>
g_volume_status storageManagerAdminRequest(const Request& request, g_volume_admin_response* response)
{
	static_assert(std::is_standard_layout<Request>::value && std::is_trivially_copyable<Request>::value,
				  "administrative requests have a fixed wire layout");
	static_assert(offsetof(Request, header) == 0, "administrative requests begin with their common header");
	return storageManagerAdminRequest(&request, sizeof(request), response);
}

template<typename Request>
g_volume_status storageManagerAdminStart(const Request& request, g_volume_job_info* info)
{
	static_assert(std::is_standard_layout<Request>::value && std::is_trivially_copyable<Request>::value,
				  "administrative requests have a fixed wire layout");
	static_assert(offsetof(Request, header) == 0, "administrative requests begin with their common header");
	return storageManagerAdminStart(&request, sizeof(request), info);
}

g_volume_status storageManagerJobGet(g_volume_job_id job, g_volume_job_info* info);
g_volume_status storageManagerFormatterRegister(const char* filesystem);

g_volume_status diskGet(g_device_id device, g_disk_info* info);
g_volume_status diskNext(g_device_id after, g_disk_info* info);
g_volume_status diskPartitionNext(g_device_id device, uint32_t after, g_disk_partition_info* info);

/**
 * Return a validated boot area and the existing MBR identity, or the supplied
 * nonzero identity for a disk without one. The caller must hold the disk claim.
 */
g_volume_status diskBootGet(g_device_id device, uint64_t claim, g_volume_id volume, uint64_t sectors, uint32_t identity,
							g_disk_boot_info* info);

/**
 * Start guarded boot writes as ordinary administrative jobs. Chunks contain
 * whole sectors outside sector zero; commit accepts exactly the MBR boot code.
 * Retain the returned identity and generation from diskBootGet throughout.
 */
g_volume_status diskBootWrite(const g_disk_boot_info& boot, uint64_t first, const void* bytes, size_t size,
							  g_volume_job_info* job);
g_volume_status diskBootCommit(const g_disk_boot_info& boot, const void* code, size_t size, bool activate,
							   g_volume_job_info* job);
#endif
