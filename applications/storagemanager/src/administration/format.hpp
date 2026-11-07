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

#ifndef STORAGEMANAGER_ADMIN_FORMAT
#define STORAGEMANAGER_ADMIN_FORMAT

#include "jobs.hpp"

/**
 * Registers sender as the FAT32 formatter, rejecting a competing live formatter.
 */
g_volume_status adminFormatRegister(g_tid sender, const g_volume_formatter_register_request* request);

/**
 * Validates format parameters and target layout, then captures the registered
 * formatter for the job. The caller supplies the validated target's scheme.
 */
g_volume_status adminFormatPrepare(g_volume_admin_job* job, g_disk_layout layout, g_volume_scheme scheme);

/**
 * Sends the bounded format job to its registered formatter, authenticates
 * progress and completion replies, and acknowledges the formatter's worker.
 */
g_volume_status adminFormatRun(g_volume_admin_job* job);

#endif
