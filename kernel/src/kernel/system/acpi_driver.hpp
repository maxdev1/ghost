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

#ifndef GHOST_KERNEL_ACPI_DRIVER_HPP
#define GHOST_KERNEL_ACPI_DRIVER_HPP

#include <ghost/system/acpi.h>
#include <ghost/tasks/types.h>

/**
 * Finds the registered ACPI service if its task is live and has driver privilege.
 */
g_tid acpiDriverGetService();

/**
 * Waits for the ACPI service and requests its firmware capabilities.
 * May block; call from task context while the scheduler is running.
 */
bool acpiDriverGetInfo(g_acpi_info* info);

/**
 * Prepares or cancels a firmware operation through the ACPI service.
 * Both calls must use the same nonzero token and calling task. They may block
 * and return false on timeout, invalid replies, driver exit or refusal.
 */
bool acpiDriverPrepare(g_power_action action, uint32_t token, g_acpi_power_plan* plan);

bool acpiDriverCancel(g_power_action action, uint32_t token);

#endif
