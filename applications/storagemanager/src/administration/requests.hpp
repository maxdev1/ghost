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

#ifndef STORAGEMANAGER_ADMIN_REQUESTS
#define STORAGEMANAGER_ADMIN_REQUESTS

#include <ghost/messages.h>
#include "disk/storage.hpp"

/**
 * An owned administrative request. Decoding retains only the operation's data;
 * unused fields stay zero and the worker never refers to the receive buffer.
 */
struct g_volume_admin_request
{
	g_volume_admin_command command;
	uint64_t operation;
	uint64_t generation;
	g_device_id device;
	g_volume_id volume;

	g_disk_layout layout;
	uint32_t flags;
	uint32_t partition;
	g_disk_partition_options partitionCreate;

	char filesystem[16];
	char label[12];

	g_disk_boot_info boot;
	uint64_t firstSector;
	uint32_t dataSize;
	uint8_t data[G_DISK_BOOT_CHUNK_SIZE];
};

struct g_volume_registry;

/**
 * Validates an administrative request, dispatches it and replies to its sender.
 */
void adminRequestHandle(g_volume_registry* registry, const g_message_header* message, const g_volume_admin_header* header);

/**
 * Compares operation identity and all retained request fields so retries can
 * reuse a job only when the complete operation matches.
 */
bool adminRequestMatches(const g_volume_admin_request* left, const g_volume_admin_request* right);

/**
 * Identifies operations that intentionally erase or replace the disk layout.
 */
bool adminRequestDiscardsLayout(const g_volume_admin_request* request);

#endif
