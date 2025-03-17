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

#ifndef __LIBDEVICE_INTERFACE__
#define __LIBDEVICE_INTERFACE__

#include <cstdint>
#include <ghost/tasks/types.h>

#define G_DEVICE_MANAGER_NAME		"devicemanager"

/**
 * Different device types the system can handle
 */
typedef uint16_t g_device_type;
#define G_DEVICE_TYPE_VIDEO		((g_device_type) 1)
#define G_DEVICE_TYPE_BLOCK		((g_device_type) 2)

/**
 * System wide device identifier given by the device manager on registration
 */
typedef uint16_t g_device_id;

/**
 * Commands that the device manager understands
 */
typedef uint16_t g_device_manager_command;
#define G_DEVICE_MANAGER_REGISTER_DEVICE	((g_device_manager_command) 0)

/**
 * Generic response codes
 */
typedef uint8_t g_device_manager_general_status;
#define G_DEVICE_MANAGER_SUCCESS	((g_device_manager_general_status) 1)
#define G_DEVICE_MANAGER_ERROR		((g_device_manager_general_status) 2)

/**
 * Header of each request
 */
struct g_device_manager_header
{
	g_device_manager_command command;
}__attribute__((packed));

struct g_device_manager_register_device_request
{
	g_device_manager_header header;
	g_device_type type;
	g_tid handler;
}__attribute__((packed));

struct g_device_manager_register_device_response
{
	g_device_manager_general_status status;
	g_device_id id;
}__attribute__((packed));

/**
 * Messages that the manager posts to the device topic
*/
#define G_DEVICE_EVENT_TOPIC		"device"

typedef uint16_t g_device_event;
#define G_DEVICE_EVENT_DEVICE_REGISTERED	((g_device_event) 0)

struct g_device_event_header
{
	g_device_event event;
}__attribute__((packed));

struct g_device_event_device_registered
{
	g_device_event_header header;
	g_device_id id;
	g_device_type type;
	g_tid driver;
}__attribute__((packed));


#endif
