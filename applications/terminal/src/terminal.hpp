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

#ifndef __TERMINAL__
#define __TERMINAL__

#include "screen/screen.hpp"
#include "stream_status.hpp"

class terminal_t;

/**
 * Information struct used to pass information to the output thread.
 */
struct output_routine_startinfo_t
{
	bool isStderr;
};

/**
 * Initializes the terminals screen. If successful, the terminals screen property
 * is set to the new screen, otherwise it remains null.
 */
void terminalInitializeScreen();

/**
 * Starts the shell process, mapping the in/out/error pipes to this thread.
 */
bool terminalStartShell();

void terminalWriteToShell(std::string line);
void terminalWriteTermkeyToShell(int termkey);

/**
 * Thread that continuously reads the keyboard input, processes and redirects
 * it to the shell.
 */
void terminalInputRoutine();

/**
 * Thread that reads the output of the executing program and processes it.
 */
void terminalOutputRoutine(output_routine_startinfo_t* info);

void terminalProcessOutput(stream_control_status_t* status, bool error_stream, char c);
void terminalProcessSequenceVt100(stream_control_status_t* status);
static screen_color_t terminalColorFromVt100(int color);
void terminalProcessSequenceGhostterm(stream_control_status_t* status);

#endif
