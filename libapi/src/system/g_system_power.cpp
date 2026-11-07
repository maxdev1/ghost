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

#include "ghost/syscall.h"
#include "ghost/system.h"

g_power_status g_system_power(g_power_action action)
{
	g_syscall_system_power data{};
	data.operation = G_POWER_CALL_REQUEST;
	data.action = action;
	data.status = G_POWER_NOT_SUPPORTED;
	g_syscall(G_SYSCALL_SYSTEM_POWER, (g_address) &data);
	return data.status;
}

g_power_status g_power_register(g_power_role role)
{
	g_syscall_system_power data{};
	data.operation = G_POWER_CALL_REGISTER;
	data.role = role;
	data.status = G_POWER_NOT_SUPPORTED;
	g_syscall(G_SYSCALL_SYSTEM_POWER, (g_address) &data);
	return data.status;
}

g_power_status g_power_acknowledge(uint32_t request, uint8_t ready)
{
	g_syscall_system_power data{};
	data.operation = G_POWER_CALL_ACKNOWLEDGE;
	data.request = request;
	data.ready = ready;
	data.status = G_POWER_NOT_SUPPORTED;
	g_syscall(G_SYSCALL_SYSTEM_POWER, (g_address) &data);
	return data.status;
}

const char* g_power_status_string(g_power_status status)
{
	switch(status)
	{
		case G_POWER_SUCCESSFUL:
			return "Success";
		case G_POWER_BUSY:
			return "Shutdown already in progress";
		case G_POWER_NOT_SUPPORTED:
			return "Firmware does not support this operation";
		case G_POWER_CANCELLED:
			return "An application cancelled shutdown";
		case G_POWER_IO_ERROR:
			return "Could not flush storage; shutdown cancelled";
		case G_POWER_TIMED_OUT:
			return "Shutdown preparation timed out";
		case G_POWER_HARDWARE_ERROR:
			return "Hardware power operation failed";
		case G_POWER_NOT_PERMITTED:
			return "Permission denied";
		default:
			return "Invalid power request";
	}
}
