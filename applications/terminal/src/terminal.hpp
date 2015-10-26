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

#include <screen.hpp>
#include <standard_io.hpp>

#include <ghost.h>
#include <stdint.h>
#include <vector>

/**
 *
 */
typedef int terminal_input_status_t;
const terminal_input_status_t TERMINAL_INPUT_STATUS_DEFAULT = 0;
const terminal_input_status_t TERMINAL_INPUT_STATUS_EXIT = 1;
const terminal_input_status_t TERMINAL_INPUT_STATUS_SCREEN_SWITCH = 2;
const terminal_input_status_t TERMINAL_INPUT_STATUS_SCREEN_CREATE = 3;

#define BUILTIN_COMMAND_CD			"cd "
#define BUILTIN_COMMAND_SLEEP		"sleep "
#define BUILTIN_COMMAND_CLEAR		"clear"
#define BUILTIN_COMMAND_TERM		"terminal"
#define BUILTIN_COMMAND_TERM_P		"terminal "
#define BUILTIN_COMMAND_TERMS		"terminals"
#define BUILTIN_COMMAND_BACKGROUND	"background "
#define BUILTIN_COMMAND_KBD_SET		"keyboard set "
#define BUILTIN_COMMAND_KBD_INFO	"keyboard info"
#define BUILTIN_COMMAND_SCANCODE	"scancode"
#define BUILTIN_COMMAND_HELP		"help"

/**
 *
 */
typedef struct {
	int number;
} create_terminal_info_t;

extern int terminal_index;

/**
 *
 */
class terminal_t {
public:
	/**
	 * Sets up the terminal, loading the default keyboard layout
	 */
	static void prepare();

	/**
	 * Creates a terminal.
	 */
	static void create_terminal(create_terminal_info_t* inf);

	/**
	 * Main loop of the terminal. Reads the entered commands, tries to
	 * handle them as built-in commands and otherwise tries to execute the
	 * executable in the current directory.
	 */
	void run(create_terminal_info_t* inf);

	/**
	 * Tries to handle the command as a built-in command.
	 *
	 * @param command
	 * 		the command to handle
	 *
	 * @return whether the command was a builtin command
	 */
	bool handle_builtin(std::string command);

	/**
	 *
	 */
	void run_command(std::string command);
	bool run_term_command(std::string command, g_fd* term_in, g_fd* term_out,
			g_fd* term_err, g_pid* int_pid);

	/**
	 *
	 */
	bool execute(std::string shortpath, std::string args, g_pid* out_pid,
			g_fd out_stdio[3], g_fd in_stdio[3]);

	g_set_working_directory_status write_working_directory();
	void read_working_directory();

	static void switch_to_next();
	static void switch_to(terminal_t* terminal);
	static std::string read_input(std::string there, screen_t* screen,
			terminal_input_status_t* out_status, bool* do_break);

	static void standard_in_thread(standard_in_thread_data_t* data);
	static void standard_out_thread(standard_out_thread_data_t* data);

	bool file_exists(std::string path);
	bool find_in_path(std::string path, std::string& out);

private:
	std::string working_directory;
	screen_t* screen;
	uint8_t inactive;

	static void add_terminal(create_terminal_info_t* inf);
	terminal_t();

	static screen_t* addScreen();
};

#endif
