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

#ifndef STORAGEMANAGER_DISCOVERY
#define STORAGEMANAGER_DISCOVERY

#include <ghost/messages.h>
#include <libdevice/interface.hpp>
#include "registry.hpp"

/**
 * Refreshes disk geometry and layout, reconciles volumes and publishes events.
 * Uses requester and job to check reservations; manual scans request probing
 * without automatic mounting.
 */
g_volume_status registryDiscoveryScanDisk(g_volume_registry* registry, const g_volume_info* disk, g_tid requester, g_volume_job_id job, bool manual);

/**
 * Validates a block-device registration event from the device manager and
 * scans the device if it is not already registered.
 */
void registryDiscoveryHandleDevice(g_volume_registry* registry, g_tid manager, const g_message_header* message,
					   const g_device_event_device_registered* event);

#endif
