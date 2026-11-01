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

#ifndef STORAGEMANAGER_ADMINISTRATION
#define STORAGEMANAGER_ADMINISTRATION
#include <ghost/messages.h>
#include "registry.hpp"

using g_volume_scan_function = g_volume_status (*)(g_volume_info, g_tid, g_volume_job_id, bool);

void storageManagerAdminInitialize(g_volume_scan_function scan);

bool storageManagerAdminReceive(g_volume_registry* registry, const g_message_header* message, const void* content);

void storageManagerAdminPoll(g_volume_registry* registry);

bool storageManagerAdminQuiescing();

void storageManagerAdminPollPower(bool controlsPending);

#endif
