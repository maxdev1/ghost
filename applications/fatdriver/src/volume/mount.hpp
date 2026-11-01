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

#ifndef FATDRIVER_MOUNT
#define FATDRIVER_MOUNT

#include "fatdriver.hpp"

bool fatMountConfigureRoot(const char* specification);

/**
 * Flushes a healthy writable mount. Read-only mounts have nothing to flush.
 */
bool fatMountFlush(g_fat_mount* mount);
g_volume_status fatMountFinishUnmount(g_fat_mount* mount);

/**
 * Consumes messages from the storage manager, including invalid control requests.
 * Other messages remain available to the filesystem delegate dispatcher.
 */
bool fatMountReceiveControl(g_fat_mount* mount, const g_message_header* message, const void* content);
void fatDiscoverVolumes();

#endif
