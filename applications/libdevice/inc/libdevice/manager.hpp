/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2025, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __LIBDEVICE_MANAGER__
#define __LIBDEVICE_MANAGER__

#include "interface.hpp"

/**
 * Tells the device manager that a new device should be added and which task
 * is responsible for handling interaction with the device.
 */
bool deviceManagerRegisterDevice(g_device_type type, g_tid handler, g_device_id* outId);

/**
 * Queries one page of registered devices in ascending ID order. Pass zero for
 * afterId initially, then the last returned ID. A short page ends the list.
 * Returns false if the manager is unavailable or the exchange fails.
 */
bool deviceManagerListDevices(g_device_id afterId, g_device_manager_list_devices_response* outResponse);

#endif
