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

#ifndef __TERMINAL_SCREEN_STREAM_STATUS__
#define __TERMINAL_SCREEN_STREAM_STATUS__

#include <ghostuser/io/terminal.hpp>

/**
 * Determines the state of a stream to the terminal.
 */
typedef int terminal_stream_status_t;
const terminal_stream_status_t TERMINAL_STREAM_STATUS_TEXT = 0;
const terminal_stream_status_t TERMINAL_STREAM_STATUS_LAST_WAS_ESC = 1;
const terminal_stream_status_t TERMINAL_STREAM_STATUS_WITHIN_VT100 = 2;
const terminal_stream_status_t TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM = 3;

/**
 * Used to remember the status of a stream, for example whether a control
 * sequence is currently being sent.
 */
typedef struct {
	terminal_stream_status_t status = TERMINAL_STREAM_STATUS_TEXT;
	int parameterCount = 0;
	int parameters[TERMINAL_STREAM_CONTROL_MAX_PARAMETERS];
	char controlCharacter;
} stream_control_status_t;

#endif
