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

#ifndef GHOST_BUILDCONFIGURATION
#define GHOST_BUILDCONFIGURATION

#include <logger/log_level.hpp>

#if !(defined(_ARCH_X86_) || defined(_ARCH_X86_64_))
#error "No architecture defined for build. Need to specify _ARCH_X86_ or _ARCH_x86_64_"
#endif

/**
 * Log level
 */
#define G_LOG_LEVEL										G_LOG_LEVEL_WARN

/**
 * Version number
 */
#define G_VERSION_MAJOR									0
#define G_VERSION_MINOR									4
#define G_VERSION_PATCH									2

/**
 * Name of the initialization binary that the kernel loads on startup
 */
#define G_INIT_BINARY_NAME								"applications/init.bin"
#define G_IDLE_BINARY_NAME								"applications/idle.bin"

/**
 * Detail debugging
 */
#define G_DEBUG_SYSCALLS								false
#define G_DEBUG_WHOS_WAITING							false
#define G_DEBUG_LOCKS_DEADLOCKING						false

#endif
