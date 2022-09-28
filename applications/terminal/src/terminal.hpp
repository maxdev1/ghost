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

#ifndef __GHOST_TERMINAL__
#define __GHOST_TERMINAL__

#include <ghost.h>
#include <libterminal/terminal.hpp>
#include <screen.hpp>
#include <stream_status.hpp>

class terminal_t;

/**
 * Information struct used to pass information to the output thread.
 */
struct output_routine_startinfo_t
{
	bool error_output;
	terminal_t* terminal;
};

/**
 *
 */
class terminal_t
{
  private:
	g_fd shell_in;
	g_fd shell_out;
	g_fd shell_err;

	/**
	 * Screen that is the visual interface to the user.
	 */
	screen_t* screen;
	g_atom screen_lock = 0;

	/**
	 * Mode flags
	 */
	bool echo;
	g_terminal_mode input_mode;

	/**
	 * ID of the currently controlled process
	 */
	g_pid current_process;

  public:
	/**
	 *
	 */
	terminal_t() : screen(0), shell_in(G_FD_NONE), shell_out(G_FD_NONE), shell_err(G_FD_NONE),
				   echo(true), input_mode(G_TERMINAL_MODE_DEFAULT), current_process(-1)
	{
	}

	/**
	 * Starts the terminal application.
	 */
	void execute();

	/**
	 * Initializes the terminals screen. If successful, the terminals screen property
	 * is set to the new screen, otherwise it remains null.
	 */
	void initializeScreen();

	/**
	 * Starts the shell process, mapping the in/out/error pipes to this thread.
	 */
	void start_shell();

	/**
	 *
	 */
	void write_string_to_shell(std::string line);

	/**
	 *
	 */
	void write_termkey_to_shell(int termkey);

	/**
	 * Thread that continuously reads the keyboard input, processes and redirects
	 * it to the shell.
	 */
	void input_routine();

	/**
	 * Thread that reads the output of the executing program and processes it.
	 */
	static void output_routine(output_routine_startinfo_t* info);

	/**
	 *
	 */
	void process_output_character(stream_control_status_t* status, bool error_stream, char c);
	void process_vt100_sequence(stream_control_status_t* status);
	static screen_color_t convert_vt100_to_screen_color(int color);
	void process_ghostterm_sequence(stream_control_status_t* status);
};

#endif
