/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max SchlÃƒÂ¼ssel <lokoxe@gmail.com>                     *
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

#include <terminal.hpp>
#include <headless_screen.hpp>
#include <gui_screen.hpp>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <fstream>
#include <vector>

#include <ghost.h>
#include <ghostuser/utils/utils.hpp>
#include <ghostuser/utils/logger.hpp>
#include <ghostuser/io/keyboard.hpp>
#include <ghostuser/ui/ui.hpp>
#include <dirent.h>

static std::vector<terminal_t*> terminals;
static std::vector<g_tid> terminal_threads;
static terminal_t* active_terminal = 0;

int terminal_index = 0;
static bool headless;

/**
 *
 */
int main(int argc, char* argv[]) {

	// check for headless argument
	headless = (strcmp("--headless", argv[1]) == 0);

	// headless may only run once
	if (g_task_get_id("terminal_headless") != -1) {
		std::cerr
				<< "error: terminal can only be executed once when in headless mode"
				<< std::endl;
		return -1;
	}

	// if not headless, init gui
	if (!headless) {
		auto status = g_ui::open();

		if (status != G_UI_OPEN_STATUS_SUCCESSFUL) {
			klog("failed to open UI for terminal with status %i", status);
			return -1;
		}
	}

	// initialize general settings
	terminal_t::prepare();

	// instantiate a terminal
	create_terminal_info_t* inf = new create_terminal_info_t;
	inf->number = terminal_index++;
	terminal_t::create_terminal(inf);

	// join all terminals
	while (terminal_threads.size() > 0) {
		g_join(terminal_threads[0]);
	}
}

/**
 *
 */
bool terminal_t::runs_headless() {
	return headless;
}

/**
 * Prepares the terminal application for execution. Loads the current keyboard
 * layout and disables video log to avoid interfering with headless mode.
 */
void terminal_t::prepare() {

	// register as the terminal task when headless
	if (headless) {
		g_task_register_id("terminal_headless");
		klog("initializing headless terminal");
	}

	// load keyboard layout
	std::string initialLayout = "de-DE";
	if (g_keyboard::loadLayout(initialLayout)) {
		g_logger::log("keyboard layout '" + initialLayout + "' loaded");
	} else {
		g_logger::log("unable to load keyboard layout '" + initialLayout + "'");
		return;
	}

	// disable video logging
	g_set_video_log(false);
}

/**
 * Creates a new terminal by starting a thread.
 */
void terminal_t::create_terminal(create_terminal_info_t* inf) {
	g_tid term = g_create_thread_d((void*) &terminal_t::terminal_start_routine,
			(void*) inf);
	terminal_threads.push_back(term);
}

/**
 *
 */
void terminal_t::terminal_start_routine(create_terminal_info_t* inf) {

	// create new terminal
	terminal_t* terminal = new terminal_t();
	terminals.push_back(terminal);

	// copy working directory
	if (active_terminal) {
		terminal->working_directory = active_terminal->working_directory;
	}

	// switch screen to new terminal
	switch_to(terminal);

	// run terminal in this thread
	terminal->run(inf);
}

/**
 *
 */
terminal_t::terminal_t() :
		inactive(false) {
	if (headless) {
		screen = new headless_screen_t();
	} else {
		screen = new gui_screen_t();
	}
}

/**
 *
 */
void terminal_t::switch_to_next() {

	terminal_t* next = 0;

	bool found_current = false;
	for (terminal_t* other : terminals) {
		if (other == active_terminal) {
			found_current = true;
		} else if (found_current) {
			next = other;
			break;
		}
	}

	if (next == 0) {
		next = terminals[0];
	}

	switch_to(next);
}

/**
 *
 */
void terminal_t::switch_to(terminal_t* term) {

	if (active_terminal) {
		active_terminal->inactive = true;
		active_terminal->screen->deactivate();
		active_terminal->read_working_directory();
	}

	active_terminal = term;

	active_terminal->write_working_directory();
	active_terminal->inactive = false;
	active_terminal->screen->activate();
	active_terminal->screen->updateCursor();
}

/**
 *
 */
g_set_working_directory_status terminal_t::write_working_directory() {
	return g_set_working_directory(working_directory.c_str());
}

/**
 *
 */
void terminal_t::read_working_directory() {

	char* buf = new char[G_PATH_MAX];
	g_get_working_directory(buf);
	working_directory = std::string(buf);
	delete buf;
	screen->workingDirectoryChanged(working_directory);
}

/**
 *
 */
void terminal_t::writeToScreen(screen_t* screen, std::string str,
		screen_color_t background, screen_color_t foreground) {

	screen->lock();

	int bg = screen->getColorBackground();
	int fg = screen->getColorForeground();
	screen->setColorBackground(background);
	screen->setColorForeground(foreground);

	const char* c = str.c_str();
	while (*c) {
		screen->writeChar(*c);
		++c;
	}

	screen->setColorBackground(bg);
	screen->setColorForeground(fg);

	screen->unlock();
}

/**
 *
 */
void terminal_t::run(create_terminal_info_t* inf) {

	read_working_directory();
	screen->clean();
	screen->activate();

	if (headless) {
		if (inf->number == 0) {
			// print the logo
			std::ifstream logofile("/system/graphics/logo.oem-us");
			writeToScreen(screen, "\n");

			if (logofile.good()) {
				std::string logo((std::istreambuf_iterator<char>(logofile)),
						std::istreambuf_iterator<char>());
				writeToScreen(screen, logo, SC_BLACK, SC_LBLUE);
			}

			std::stringstream msg1;
			msg1 << std::endl;
			msg1 << " Copyright (c) 2012-2016 Max Schl" << OEMUS_CHAR_UE
					<< "ssel <lokoxe@gmail.com>" << std::endl;
			writeToScreen(screen, msg1.str(), SC_BLACK, SC_LGRAY);

			std::stringstream msg2;
			msg2 << " Enter 'read README' for a brief introduction.";
			msg2 << std::endl << std::endl;
			writeToScreen(screen, msg2.str(), SC_BLACK, SC_WHITE);

		} else {
			std::stringstream msg;
			msg << "terminal " << inf->number << std::endl;
			writeToScreen(screen, msg.str());
		}
	}

	// delete info
	delete inf;

	std::string command;

	bool write_header = true;
	while (true) {
		// wait for activity
		g_atomic_block(&inactive);
		screen->updateCursor();

		if (write_header) {
			// write current directory
			writeToScreen(screen, working_directory, SC_BLACK, SC_LGRAY);
			writeToScreen(screen, ">", SC_BLACK, SC_DARKGRAY);
			screen->updateCursor();
		} else {
			write_header = true;
		}

		// read line
		terminal_input_status_t status;
		command = read_input(command, this, &status, 0);

		// handle input
		if (status == TERMINAL_INPUT_STATUS_SCREEN_SWITCH) {
			switch_to_next();
			write_header = false;

		} else if (status == TERMINAL_INPUT_STATUS_SCREEN_CREATE) {

			create_terminal_info_t* inf = new create_terminal_info_t;
			inf->number = terminal_index++;
			create_terminal(inf);
			write_header = false;

		} else if (status == TERMINAL_INPUT_STATUS_EXIT) {
			write_header = false;

		} else if (status == TERMINAL_INPUT_STATUS_DEFAULT) {
			writeToScreen(screen, "\n");
			screen->updateCursor();

			if (!command.empty()) {
				if (!handle_builtin(command)) {
					run_command(command);
				}
			}

			command = "";
		}
	}
}

/**
 *
 */
void terminal_t::run_command(std::string command) {

	g_fd term_in;
	g_fd term_out;
	g_fd term_err;
	g_pid last_pid;
	if (!run_term_command(command, &term_in, &term_out, &term_err, &last_pid)) {
		return;
	}

	standard_in_thread_data_t in_data;
	in_data.continue_input = true;
	in_data.stdin_write_end = term_in;
	in_data.terminal = this;
	g_tid rin = g_create_thread_d((void*) &standard_in_thread,
			(void*) &in_data);

	standard_out_thread_data_t out_data;
	out_data.stop = false;
	out_data.err = false;
	out_data.stdout_read_end = term_out;
	out_data.terminal = this;
	out_data.screen = screen;
	g_tid rout = g_create_thread_d((void*) &standard_out_thread,
			(void*) &out_data);

	standard_out_thread_data_t err_data;
	err_data.stop = false;
	err_data.err = true;
	err_data.stdout_read_end = term_err;
	out_data.terminal = this;
	err_data.screen = screen;
	g_tid rerr = g_create_thread_d((void*) &standard_out_thread,
			(void*) &err_data);

	in_data.int_pid = last_pid;

	// wait for process to exit
	g_join(last_pid);
	in_data.continue_input = false;
	out_data.stop = true;
	err_data.stop = true;

	// wait for input thread before leaving
	g_join(rin);
	g_join(rout);
	g_join(rerr);
}

/**
 *
 */
bool terminal_t::run_term_command(std::string command, g_fd* term_in,
		g_fd* term_out, g_fd* term_err, g_pid* int_pid) {

	// create initial pipe
	g_fd first_pipe_w;
	g_fd first_pipe_r;
	g_fs_pipe_status first_pipe_stat = g_pipe(&first_pipe_w, &first_pipe_r);
	if (first_pipe_stat != G_FS_PIPE_SUCCESSFUL) {
		writeToScreen(screen, "unable to create process pipe",
				SC_COLOR(SC_BLACK, SC_RED));
		return false;
	}
	*term_in = first_pipe_w;
	g_fd last_stdout = first_pipe_r;

	// split by pipes
	std::string part;
	std::vector<std::string> parts;
	std::stringstream commandstr(command);
	while (std::getline(commandstr, part, '|')) {
		parts.push_back(part);
	}

	int numparts = parts.size();
	for (int i = 0; i < numparts; i++) {

		// split by path and arguments
		std::string path = g_utils::trim(parts[i]);
		std::string args = "";

		int spacePos = path.find(' ');
		if (spacePos != -1) {
			args = path.substr(spacePos + 1);
			path = path.substr(0, spacePos);
		}

		// create pipes
		g_fd out_pipe_w;
		g_fd out_pipe_r;
		g_fs_pipe_status pipe_stat = g_pipe(&out_pipe_w, &out_pipe_r);
		if (pipe_stat != G_FS_PIPE_SUCCESSFUL) {
			writeToScreen(screen, "unable to create process out pipe",
					SC_COLOR(SC_BLACK, SC_RED));
			return false;
		}

		// execute it
		g_pid spawned_pid;
		g_fd spawned_stdio[3];
		g_fd in_stdio[3];
		in_stdio[0] = last_stdout;
		in_stdio[1] = out_pipe_w;
		in_stdio[2] = G_FD_NONE;

		if (i == numparts - 1) { // stderr only for last
			g_fd err_pipe_w;
			g_fd err_pipe_r;
			pipe_stat = g_pipe(&err_pipe_w, &err_pipe_r);
			if (pipe_stat != G_FS_PIPE_SUCCESSFUL) {
				writeToScreen(screen, "unable to create process err pipe",
						SC_COLOR(SC_BLACK, SC_RED));
				return false;
			}
			in_stdio[2] = err_pipe_w;
			*term_err = err_pipe_r;
		}

		if (!execute(path, args, &spawned_pid, spawned_stdio, in_stdio)) {
			// TODO cleanup pipes
			return false;
		}

		// close stderr in last
		if (i == numparts - 1) {
			g_close(in_stdio[2]);
		}

		// close write end of this pipe in this process
		g_close(out_pipe_w);

		// close read end of previous pipe in this process
		g_close(last_stdout);

		// next one uses this as output
		last_stdout = out_pipe_r;

		// last is the one that's interrupted
		*int_pid = spawned_pid;
	}

	// last is used as terminal output
	*term_out = last_stdout;

	return true;
}

/**
 *
 */
bool terminal_t::handle_builtin(std::string command) {

	if (command == BUILTIN_COMMAND_HELP) {
		writeToScreen(screen,
				" The terminal has the following built-in functions:\n");
		writeToScreen(screen, "\n");
		writeToScreen(screen,
				" help                  prints this help screen\n");
		writeToScreen(screen,
				" ls                    lists all files in the current directory\n");
		writeToScreen(screen,
				" cd <path>             switches to a directory\n");
		writeToScreen(screen, " clear                 clears the screen\n");
		writeToScreen(screen,
				" sleep <ms>            sleeps for the given number of milliseconds\n");
		writeToScreen(screen,
				" background <file>     runs a program in the background\n");
		writeToScreen(screen, "\n");
		writeToScreen(screen, " terminal, ctrl+space  open a new terminal\n");
		writeToScreen(screen,
				" terminal <num>        switches to the terminal with the given number\n");
		writeToScreen(screen, " terminals             lists all terminals\n");
		writeToScreen(screen,
				" ctrl+tab              switches to the next terminal\n");
		writeToScreen(screen, "\n");
		writeToScreen(screen,
				" keyboard set <layout> switches the keyboard layout\n");
		writeToScreen(screen,
				" keyboard info         prints the keyboard layout\n");
		writeToScreen(screen, "\n");
		return true;

	} else if (command.substr(0,
			std::string(BUILTIN_COMMAND_KBD_SET).length()) == BUILTIN_COMMAND_KBD_SET) {
		command = command.substr(std::string(BUILTIN_COMMAND_KBD_SET).length());
		bool layoutLoaded = g_keyboard::loadLayout(command);
		if (layoutLoaded) {
			writeToScreen(screen,
					"keyboard layout to '" + command + "' set \n");
		} else {
			writeToScreen(screen,
					"keyboard layout to '" + command + "' not found\n");
		}
		return true;

	} else if (command == BUILTIN_COMMAND_SCANCODE) {

		if (headless) {
			while (true) {
				auto key = g_keyboard::readKey();
				if (key.pressed && key.ctrl && key.key == "KEY_C") {
					break;
				}
				std::stringstream msg;
				msg << "scancode: " << (key.pressed ? "d " : "u ")
						<< (uint32_t) key.scancode << " "
						<< (key.ctrl ? "+ctrl" : "") << (key.alt ? "+alt" : "")
						<< (key.shift ? "+shift" : "") << " ->" << key.key
						<< std::endl;
				writeToScreen(screen, msg.str());
			}
		} else {
			writeToScreen(screen,
					"This command is only available in headless mode.\n");
		}
		return true;

	} else if (command == BUILTIN_COMMAND_KBD_INFO) {
		writeToScreen(screen,
				"keyboard layout is '" + g_keyboard::getCurrentLayout()
						+ "'\n");
		return true;

	} else if (command.substr(0,
			std::string(BUILTIN_COMMAND_BACKGROUND).length()) == BUILTIN_COMMAND_BACKGROUND) {
		command = command.substr(
				std::string(BUILTIN_COMMAND_BACKGROUND).length());

		// TODO parse arguments
		std::string path = command;
		std::string args = "";

		int spacePos = path.find(' ');
		if (spacePos != -1) {
			args = path.substr(spacePos + 1);
			path = path.substr(0, spacePos);
		}

		g_pid pid;
		g_fd out_stdio[3];
		g_fd in_stdio[3];
		in_stdio[0] = G_FD_NONE;
		in_stdio[1] = G_FD_NONE;
		in_stdio[2] = G_FD_NONE;
		execute(path, args, &pid, out_stdio, in_stdio);

		std::stringstream msg;
		msg << "process ";
		msg << pid;
		msg << "\n";
		writeToScreen(screen, msg.str());
		return true;

	} else if (command.substr(0,
			std::string(BUILTIN_COMMAND_CD).length()) == BUILTIN_COMMAND_CD) {

		working_directory = command.substr(
				std::string(BUILTIN_COMMAND_CD).length());
		g_set_working_directory_status stat = write_working_directory();

		if (stat == G_SET_WORKING_DIRECTORY_NOT_A_FOLDER) {
			writeToScreen(screen, "selected file not a folder\n",
					SC_COLOR(SC_BLACK, SC_RED));
		} else if (stat == G_SET_WORKING_DIRECTORY_NOT_FOUND) {
			writeToScreen(screen, "directory not found\n",
					SC_COLOR(SC_BLACK, SC_RED));
		} else if (stat == G_SET_WORKING_DIRECTORY_ERROR) {
			writeToScreen(screen,
					"unable to switch to the selected directory\n",
					SC_COLOR(SC_BLACK, SC_RED));
		}

		read_working_directory();
		return true;

	} else if (command.substr(0,
			std::string(BUILTIN_COMMAND_SLEEP).length()) == BUILTIN_COMMAND_SLEEP) {
		std::string sleepTime = command.substr(
				std::string(BUILTIN_COMMAND_SLEEP).length());
		long ms;
		std::stringstream scrn;
		scrn << sleepTime;
		scrn >> ms;

		if (ms > 0 && ms < 100000) {
			std::stringstream msg;
			msg << "sleeping for ";
			msg << ms;
			msg << "ms... ";
			writeToScreen(screen, msg.str());
			g_sleep(ms);
			writeToScreen(screen, "awake!\n");
		} else {
			std::stringstream msg;
			msg << "can't sleep this long (";
			msg << ms;
			msg << "ms)\n";
			writeToScreen(screen, msg.str());
		}
		return true;

	} else if (command == BUILTIN_COMMAND_CLEAR) {
		screen->clean();
		screen->activate();
		return true;

	} else if (command == BUILTIN_COMMAND_TERMS) {
		std::stringstream msg;
		msg << terminals.size();
		msg << " terminals active\n";
		writeToScreen(screen, msg.str());
		return true;

	} else if (command.substr(0,
			std::string(BUILTIN_COMMAND_TERM_P).length()) == BUILTIN_COMMAND_TERM_P) {
		std::string screenNum = command.substr(
				std::string(BUILTIN_COMMAND_TERM_P).length());
		int pos;
		std::stringstream scrn;
		scrn << screenNum;
		scrn >> pos;

		if (pos < terminals.size() && pos >= 0) {
			switch_to(terminals[pos]);

		} else {
			std::stringstream msg;
			msg << pos;
			msg << "is not a valid screen index\n";
			writeToScreen(screen, msg.str());
		}
		return true;

	} else if (command == BUILTIN_COMMAND_TERM) {

		create_terminal_info_t* inf = new create_terminal_info_t;
		inf->number = terminal_index++;
		create_terminal(inf);
		return true;

	}

	return false;
}

/**
 *
 */
void terminal_t::standard_out_thread(standard_out_thread_data_t* data) {

	int buflen = 1024;
	char* buf = new char[buflen];

	stream_control_status_t status;

	while (true) {
		g_fs_read_status stat;
		int r = g_read_s(data->stdout_read_end, buf, buflen, &stat);

		if (stat == G_FS_READ_SUCCESSFUL) {

			for (int i = 0; i < r; i++) {
				char c = buf[i];

				// Lock screen and set error colori f required
				data->screen->lock();
				process_output_character(data, &status, c);
				data->screen->unlock();
			}

			data->screen->updateCursor();
		} else {
			break;
		}
	}

	data->screen->updateCursor();

	delete buf;
}

/**
 *
 */
void terminal_t::process_output_character(standard_out_thread_data_t* data,
		stream_control_status_t* status, char c) {

	if (status->status == TERMINAL_STREAM_STATUS_TEXT) {

		// Simple textual output
		if (c == '\r') {
			return;

		} else if (c == '\t') {
			data->screen->writeChar(' ');
			data->screen->writeChar(' ');
			data->screen->writeChar(' ');
			data->screen->writeChar(' ');

		} else if (c == 27 /* ESC */) {
			status->status = TERMINAL_STREAM_STATUS_LAST_WAS_ESC;

		} else {
			int fg = data->screen->getColorForeground();
			if (data->err) {
				data->screen->setColorForeground(SC_RED);
			}

			data->screen->writeChar(c);

			if (data->err) {
				data->screen->setColorForeground(fg);
			}
		}

		// ESC was sent
	} else if (status->status == TERMINAL_STREAM_STATUS_LAST_WAS_ESC) {
		// must be followed by [ for VT100 sequence
		if (c == '[') {
			status->status = TERMINAL_STREAM_STATUS_WITHIN_VT100;

			// or a Ghost terminal sequence
		} else if (c == '{') {
			status->status = TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM;

			// otherwise reset
		} else {
			status->status = TERMINAL_STREAM_STATUS_TEXT;
		}

		// Handle sequence parameters
	} else if (status->status == TERMINAL_STREAM_STATUS_WITHIN_VT100
			|| status->status == TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM) {

		// Parameter value
		if (c >= '0' && c <= '9') {
			if (status->parameterCount == 0) {
				status->parameterCount = 1;
			}

			if (status->parameterCount <= TERMINAL_STREAM_CONTROL_MAX_PARAMETERS) {
				status->parameters[status->parameterCount - 1] =
						status->parameters[status->parameterCount - 1] * 10;
				status->parameters[status->parameterCount - 1] += c - '0';

				// Illegal number of parameters is skipped
			}

			// Parameter seperator
		} else if (c == ';') {
			status->parameterCount++;

			// Finish character
		} else {
			status->controlCharacter = c;

			if (status->status == TERMINAL_STREAM_STATUS_WITHIN_VT100) {
				process_vt100_sequence(data, status);
			} else if (status->status
					== TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM) {
				process_ghostterm_sequence(data, status);
			}

			// reset status
			status->parameterCount = 0;
			for (int i = 0; i < TERMINAL_STREAM_CONTROL_MAX_PARAMETERS; i++) {
				status->parameters[i] = 0;
			}
			status->status = TERMINAL_STREAM_STATUS_TEXT;
		}

	}
}

/**
 *
 */
void terminal_t::process_vt100_sequence(standard_out_thread_data_t* data,
		stream_control_status_t* status) {

	switch (status->controlCharacter) {

	// Cursor up
	case 'A':
		data->screen->moveCursor(data->screen->getCursorX(),
				data->screen->getCursorY() - status->parameters[0]);
		break;

		// Cursor down
	case 'B':
		data->screen->moveCursor(data->screen->getCursorX(),
				data->screen->getCursorY() + status->parameters[0]);
		break;

		// Cursor forward
	case 'C':
		data->screen->moveCursor(
				data->screen->getCursorX() + status->parameters[0],
				data->screen->getCursorY());
		break;

		// Cursor back
	case 'D':
		data->screen->moveCursor(
				data->screen->getCursorX() - status->parameters[0],
				data->screen->getCursorY());
		break;

		// Mode setting
	case 'm':
		for (int i = 0; i < status->parameterCount; i++) {
			int param = status->parameters[i];

			// Reset
			if (param == 0) {
				data->screen->setColorBackground(SC_BLACK);
				data->screen->setColorForeground(SC_WHITE);

				// Foreground color
			} else if (param >= 30 && param < 40) {
				data->screen->setColorForeground(
						convert_vt100_to_screen_color(param - 30));

				// Background color
			} else if (param >= 40 && param < 50) {
				data->screen->setColorBackground(
						convert_vt100_to_screen_color(param - 40));
			}
		}

		break;

	}

}

/**
 *
 */
screen_color_t terminal_t::convert_vt100_to_screen_color(int color) {

	switch (color) {
	case VT100_COLOR_BLACK:
		return SC_BLACK;
	case VT100_COLOR_BLUE:
		return SC_BLUE;
	case VT100_COLOR_CYAN:
		return SC_CYAN;
	case VT100_COLOR_GREEN:
		return SC_GREEN;
	case VT100_COLOR_MAGENTA:
		return SC_MAGENTA;
	case VT100_COLOR_RED:
		return SC_RED;
	case VT100_COLOR_WHITE:
		return SC_WHITE;
	case VT100_COLOR_YELLOW:
		return SC_YELLOW;
	}
	return SC_WHITE;
}

/**
 *
 */
void terminal_t::process_ghostterm_sequence(standard_out_thread_data_t* data,
		stream_control_status_t* status) {

	switch (status->controlCharacter) {

	// Make terminal raw
	case 'r':
		data->terminal->input_raw = true;
		break;
	}

}
/**
 *
 */
void terminal_t::standard_in_thread(standard_in_thread_data_t* data) {

	std::string line;

	while (data->continue_input) {
		terminal_input_status_t stat;

		// we add up the line because it might contain content already
		line = terminal_t::read_input(line, data->terminal, &stat,
				&data->continue_input);

		if (stat == TERMINAL_INPUT_STATUS_EXIT) {
			if (data->int_pid != -1) {
				g_raise_signal(data->int_pid, SIGINT);
			}

		} else if (stat == TERMINAL_INPUT_STATUS_SCREEN_SWITCH) {
			data->terminal->switch_to_next();
			g_atomic_block(&data->terminal->inactive);

		} else if (stat == TERMINAL_INPUT_STATUS_DEFAULT) {
			line += "\n";
			data->terminal->writeToScreen(data->terminal->screen, "\n");
			data->terminal->screen->updateCursor();

			const char* lineContent = line.c_str();
			int lineLength = strlen(lineContent);

			int written = 0;
			int len = 0;
			while (written < lineLength) {
				len = write(data->stdin_write_end, &lineContent[written],
						lineLength - written);
				if (len <= 0) {
					break;
				}
				written += len;
			}

			line = "";
		}
	}
}

/**
 *
 */
bool terminal_t::file_exists(std::string path) {

	FILE* f;
	if ((f = fopen(path.c_str(), "r")) != NULL) {
		fclose(f);
		return true;
	}
	return false;
}

/**
 *
 */
bool terminal_t::find_executable(std::string path, std::string& out) {

	std::string in_cwd = working_directory + "/" + path;
	if (file_exists(in_cwd)) {
		out = in_cwd;
		return true;
	}

	if (file_exists(path)) {
		out = path;
		return true;
	}

	// TODO really lookup PATH environment variable
	std::string in_apps = "/applications/" + path;
	if (file_exists(in_apps)) {
		out = in_apps;
		return true;
	}

	// if not ending with ".bin", append it and search again
	if (path.length() < 4 || path.substr(path.length() - 4) != ".bin") {
		return find_executable(path + ".bin", out);
	}

	return false;
}

/**
 *
 */
bool terminal_t::execute(std::string shortpath, std::string args,
		g_pid* out_pid, g_fd out_stdio[3], g_fd in_stdio[3]) {

	// check for command in path
	std::string realpath;
	if (!find_executable(shortpath, realpath)) {
		writeToScreen(screen, shortpath + ": command not found\n");
		return false;
	}

	// spawn binary
	g_spawn_status status = g_spawn_poi(realpath.c_str(), args.c_str(),
			working_directory.c_str(), G_SECURITY_LEVEL_APPLICATION, out_pid,
			out_stdio, in_stdio);

	if (status == G_SPAWN_STATUS_SUCCESSFUL) {
		return true;
	}

	std::stringstream msg;
	msg << "failed to execute binary '";
	msg << realpath;
	msg << "', code: ";
	msg << status;
	msg << "\n";

	writeToScreen(screen, msg.str(), SC_BLACK, SC_RED);
	return false;
}

/**
 *
 */
std::string terminal_t::read_input(std::string there, terminal_t* term,
		terminal_input_status_t* out_status, bool* continue_input) {

	screen_t* screen = term->screen;

	std::string line = there;
	while (continue_input == 0 || *continue_input) {
		g_key_info key = screen->readInput(continue_input);

		if (key.pressed) {
			if ((key.ctrl && key.key == "KEY_C") || (key.key == "KEY_ESC")) {
				*out_status = TERMINAL_INPUT_STATUS_EXIT;
				break;

			} else if (key.ctrl && key.key == "KEY_SPACE") {
				*out_status = TERMINAL_INPUT_STATUS_SCREEN_CREATE;
				break;

			} else if (key.ctrl && key.key == "KEY_TAB") {
				*out_status = TERMINAL_INPUT_STATUS_SCREEN_SWITCH;
				break;

			} else if (key.key == "KEY_ENTER") {
				*out_status = TERMINAL_INPUT_STATUS_DEFAULT;
				break;

			} else if (key.key == "KEY_BACKSPACE") {
				if (line.length() > 0) {
					line = line.substr(0, line.length() - 1);
					screen->backspace();
					screen->updateCursor();
				}

			} else if (key.key == "KEY_TAB") {

				DIR* dir = opendir(term->working_directory.c_str());
				dirent* ent;

				// check if text contains a whitespace, find everything after that
				auto wspos = line.find(' ');
				std::string search =
						(wspos == -1) ? line : line.substr(wspos + 1);
				auto searchlen = search.length();

				// find a dirent that starts with the entered text
				while ((ent = readdir(dir)) != 0) {
					if (searchlen < ent->d_namlen
							&& strncmp(search.c_str(), ent->d_name,
									search.length()) == 0) {
						std::stringstream appendix;
						for (int i = search.length(); i < ent->d_namlen; i++) {
							appendix << ent->d_name[i];
						}
						line += appendix.str();
						writeToScreen(screen, appendix.str());
						break;
					}
				}

			} else {
				char chr = g_keyboard::charForKey(key);
				if (chr != -1) {
					line += chr;

					std::string out;
					out += chr;
					writeToScreen(screen, out);

					// put each character when raw
					if (term->input_raw) {
						break;
					}
				}
			}

			screen->updateCursor();
		}
	}

	return line;
}
