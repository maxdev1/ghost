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

#ifndef VM86_VIRTUAL8086_MONITOR
#define VM86_VIRTUAL8086_MONITOR

#include <system/processor_state.hpp>
#include <tasking/thread.hpp>

#define G_EFLAG_IF			(1 << 9)
#define G_EFLAG_VM			(1 << 17)
#define G_VALID_FLAGS		0xDFF

/**
 *
 */
enum g_virtual_monitor_handling_result {
	VIRTUAL_MONITOR_HANDLING_RESULT_SUCCESSFUL, VIRTUAL_MONITOR_HANDLING_RESULT_UNHANDLED_OPCODE, VIRTUAL_MONITOR_HANDLING_RESULT_FINISHED
};

/**
 *
 */
class g_virtual_8086_monitor {
public:
	static g_virtual_monitor_handling_result handleGpf(g_thread* thread);
};

#endif
