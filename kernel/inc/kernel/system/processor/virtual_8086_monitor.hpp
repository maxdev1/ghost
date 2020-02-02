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

#ifndef __KERNEL_PROCESSOR_VM86__
#define __KERNEL_PROCESSOR_VM86__

#include "kernel/system/processor/processor_state.hpp"
#include "kernel/tasking/tasking.hpp"

#define G_EFLAG_IF			(1 << 9)
#define G_EFLAG_VM			(1 << 17)
#define G_VALID_FLAGS		0xDFF

typedef uint8_t g_virtual_monitor_handling_result;

#define VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL			((g_virtual_monitor_handling_result) 0)
#define VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE	((g_virtual_monitor_handling_result) 1)
#define VIRTUAL_MONITOR_HANDLING_RESULT_FINISHED			((g_virtual_monitor_handling_result) 2)

g_virtual_monitor_handling_result vm86MonitorHandleGpf(g_task* task);

#endif
