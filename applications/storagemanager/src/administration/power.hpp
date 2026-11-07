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

#ifndef STORAGEMANAGER_ADMINISTRATION_POWER
#define STORAGEMANAGER_ADMINISTRATION_POWER

#include <ghost/messages.h>
#include <ghost/system/power.h>

/**
 * Updates shutdown admission state from validated kernel prepare/cancel events.
 */
void adminPowerHandleEvent(const g_message_header* message, const g_power_event* event);

/**
 * Reports whether shutdown preparation blocks new jobs and volume controls.
 */
bool adminPowerIsQuiescing();

/**
 * Acknowledges shutdown after accepted jobs and controls drain, or vetoes it
 * when a completed job left I/O safety uncertain.
 */
void adminPowerPoll(bool controlsPending);

#endif
