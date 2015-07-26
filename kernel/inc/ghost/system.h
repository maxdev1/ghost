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

#ifndef __GHOST_SYS_SYSTEM__
#define __GHOST_SYS_SYSTEM__

#include "ghost/common.h"

__BEGIN_C

/**
 * Required to provide the <g_spawn> function. The spawning process shall
 * register itself with this identifier to be accessible via ipc messaging.
 */
#define G_SPAWNER_IDENTIFIER		"spawner"

// spawner commands
#define G_SPAWN_COMMAND_SPAWN		1

// status codes for spawning
typedef int g_spawn_status;
#define G_SPAWN_STATUS_SUCCESSFUL 		((g_spawn_status) 0)
#define G_SPAWN_STATUS_IO_ERROR			((g_spawn_status) 1)
#define G_SPAWN_STATUS_MEMORY_ERROR		((g_spawn_status) 2)
#define G_SPAWN_STATUS_FORMAT_ERROR		((g_spawn_status) 3)
#define G_SPAWN_STATUS_UNKNOWN			((g_spawn_status) 4)

// length of the command line arguments buffer
#define G_CLIARGS_BUFFER_LENGTH		1024

// for <g_register_irq_handler>
typedef uint8_t g_register_irq_handler_status;
#define G_REGISTER_IRQ_HANDLER_STATUS_SUCCESSFUL		((g_register_irq_handler_status) 0)
#define G_REGISTER_IRQ_HANDLER_STATUS_NOT_PERMITTED		((g_register_irq_handler_status) 1)

// for <g_register_signal_handler>
typedef uint8_t g_register_signal_handler_status;
#define G_REGISTER_SIGNAL_HANDLER_STATUS_SUCCESSFUL		((g_register_signal_handler_status) 0)
#define G_REGISTER_SIGNAL_HANDLER_STATUS_INVALID_SIGNAL ((g_register_signal_handler_status) 1)

// for <g_raise_signal>
typedef uint8_t g_raise_signal_status;
#define G_RAISE_SIGNAL_STATUS_SUCCESSFUL				((g_raise_signal_status) 0)
#define G_RAISE_SIGNAL_STATUS_INVALID_SIGNAL 			((g_raise_signal_status) 1)
#define G_RAISE_SIGNAL_STATUS_INVALID_TARGET 			((g_raise_signal_status) 2)
__END_C

#endif
