/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef __KERNEL_POWER__
#define __KERNEL_POWER__

#include <ghost/memory/types.h>
#include <ghost/system/power.h>

struct g_task;

/**
 * Starts the BSP power coordinator after tasking and filesystems are available.
 */
void powerInitialize(g_physical_address rsdp);
void powerWaitForInitialization();
void powerTaskStopped(g_task* task);
bool powerIsReady();
g_power_status powerRequest(g_power_action action);
g_power_status powerRegister(g_task* task, g_power_role role);
g_power_status powerAcknowledge(g_task* task, uint32_t request, bool ready);

/**
 * Serializes mutating system calls against the shutdown drain barrier. Calls
 * arriving after the barrier wait for cancellation; existing calls finish first.
 */
bool powerSyscallEnter(uint32_t call);
void powerSyscallLeave(bool tracked);

/**
 * Parks secondary processors after interrupt dispatch, with no interrupt-handler
 * locks held. NMIs remain available for kernel page-table shootdowns.
 */
void powerProcessorCheckpoint();
g_power_status powerPlatformCommit(g_power_action action);

#endif
