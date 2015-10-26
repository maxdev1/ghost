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

static std::vector<terminal_t*> terminals;
static std::vector<g_tid> terminal_threads;
static terminal_t* active_terminal = 0;

int terminal_index = 0;

/**
 *
 */
int main(int argc, char* argv[]) {

	if (g_task_get_id("terminal") != -1) {
		std::cout << "error: can't execute terminal process twice" << std::endl;
	}

	terminal_t::prepare();

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
void terminal_t::prepare() {

	g_task_register_id("terminal");
	g_logger::log("starting terminal");

	// load keyboard layout
	std::string initialLayout = "de-DE";
	if (g_keyboard::loadLayout(initialLayout)) {
		g_logger::log("keyboard layout '" + initialLayout + "' loaded");
	} else {
		g_logger::log("unable to load keyboard layout '" + initialLayout + "'");
		return;
	}

	// disable video logging
	g_logger::log("disabling video log");
	g_set_video_log(false);
}

/**
 *
 */
void terminal_t::create_terminal(create_terminal_info_t* inf) {
	g_tid term = g_create_thread_d((void*) &terminal_t::add_terminal,
			(void*) inf);
	terminal_threads.push_back(term);
}

/**
 *
 */
void terminal_t::add_terminal(create_terminal_info_t* inf) {
	terminal_t* terminal = new terminal_t();
	terminals.push_back(terminal);

	if (active_terminal) {
		terminal->working_directory = active_terminal->working_directory;
	}

	switch_to(terminal);
	terminal->run(inf);
}

/**
 *
 */
terminal_t::terminal_t() :
		inactive(false) {
	screen = new screen_t();
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
}

/**
 *
 */
void terminal_t::run(create_terminal_info_t* inf) {

	read_working_directory();
	screen->clean();
	screen->activate();

	if (inf->number == 0) {
		// print the logo
		std::ifstream logofile("/system/graphics/logo.oem-us");
		screen->write("\n");
		if (logofile.good()) {
			std::string logo((std::istreambuf_iterator<char>(logofile)),
					std::istreambuf_iterator<char>());
			screen->write(logo, SC_COLOR(SC_BLACK, SC_LBLUE));
		}

		std::stringstream msg1;
		msg1 << std::endl;
		msg1 << " Copyright (c) 2012-2015 Max Schl" << OEMUS_CHAR_UE << "ssel"
				<< std::endl;
		screen->write(msg1.str(), SC_COLOR(SC_BLACK, SC_LGRAY));
		std::stringstream msg2;
		msg2 << " Enter '" << BUILTIN_COMMAND_HELP
				<< "' for a brief introduction.";
		msg2 << std::endl << std::endl;
		screen->write(msg2.str());

	} else {
		std::stringstream msg;
		msg << "terminal " << inf->number << std::endl;
		screen->write(msg.str());
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
			screen->write(working_directory, SC_COLOR(SC_BLACK, SC_LGRAY));
			screen->write(">", SC_COLOR(SC_BLACK, SC_DARKGRAY));
			screen->updateCursor();
		} else {
			write_header = true;
		}

		// read line
		bool break_condition = false;
		terminal_input_status_t status;
		command = read_input(command, screen, &status, &break_condition);

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
			screen->write("\n");
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
	in_data.stop = false;
	in_data.stdin_write_end = term_in;
	in_data.terminal = this;
	g_tid rin = g_create_thread_d((void*) &standard_in_thread,
			(void*) &in_data);

	standard_out_thread_data_t out_data;
	out_data.stop = false;
	out_data.err = false;
	out_data.stdout_read_end = term_out;
	out_data.screen = screen;
	g_tid rout = g_create_thread_d((void*) &standard_out_thread,
			(void*) &out_data);

	standard_out_thread_data_t err_data;
	err_data.stop = false;
	err_data.err = true;
	err_data.stdout_read_end = term_err;
	err_data.screen = screen;
	g_tid rerr = g_create_thread_d((void*) &standard_out_thread,
			(void*) &err_data);

	in_data.int_pid = last_pid;

	// wait for process to exit
	klog("terminal waits for %i to die", last_pid);
	g_join(last_pid);
	klog("terminal got woken up");
	in_data.stop = true;
	out_data.stop = true;
	err_data.stop = true;

	// wait for input thread before leaving
	g_join(rin);
	g_join(rout);
	g_join(rerr);

	screen->write("\n");
}

/**
 *
 */
bool terminal_t::run_term_command(std::string command, g_fd* term_in,
		g_fd* term_out, g_fd* term_err, g_pid* int_pid) {

	// create initial pipe
	g_fs_pipe_status first_pipe_stat;
	g_fd first_pipe_w;
	g_fd first_pipe_r;
	g_pipe_s(&first_pipe_w, &first_pipe_r, &first_pipe_stat);
	if (first_pipe_stat != G_FS_PIPE_SUCCESSFUL) {
		screen->write("unable to create process pipe", SC_ERROR_COLOR);
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
		g_fs_pipe_status pipe_stat;

		g_fd out_pipe_w;
		g_fd out_pipe_r;
		g_pipe_s(&out_pipe_w, &out_pipe_r, &pipe_stat);
		if (pipe_stat != G_FS_PIPE_SUCCESSFUL) {
			screen->write("unable to create process out pipe", SC_ERROR_COLOR);
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
			g_pipe_s(&err_pipe_w, &err_pipe_r, &pipe_stat);
			if (pipe_stat != G_FS_PIPE_SUCCESSFUL) {
				screen->write("unable to create process err pipe",
				SC_ERROR_COLOR);
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
		screen->write(" help               prints this help screen\n");
		screen->write(
				" ls                 lists all files in the current directory\n");
		screen->write(" cd <path>          switches to a directory\n");
		screen->write(" clear              clears the screen\n");
		screen->write(" keyboard set <layout> switches the keyboard layout\n");
		screen->write(" keyboard info      prints the keyboard layout\n");
		screen->write(" read               prints file contents\n");
		screen->write(
				" sleep <ms>         sleeps for the given number of milliseconds\n");
		screen->write(" background <file>  runs a program in the background\n");
		screen->write(" terminal           opens a new terminal\n");
		screen->write(
				" terminal <num>     switches to the terminal with the given number\n");
		screen->write(" terminals          lists all terminals\n");
		screen->write("\n");
		screen->write(" ctrl + tab         switches to the next terminal\n");
		screen->write("\n");
		return true;

	} else if (command.substr(0,
			std::string(BUILTIN_COMMAND_KBD_SET).length()) == BUILTIN_COMMAND_KBD_SET) {
		command = command.substr(std::string(BUILTIN_COMMAND_KBD_SET).length());
		bool layoutLoaded = g_keyboard::loadLayout(command);
		if (layoutLoaded) {
			screen->write("keyboard layout to '" + command + "' set \n");
		} else {
			screen->write("keyboard layout to '" + command + "' not found\n");
		}
		return true;

	} else if (command == BUILTIN_COMMAND_SCANCODE) {
		while (true) {
			auto key = g_keyboard::readKey();
			if (key.pressed && key.ctrl && key.key == "KEY_C") {
				break;
			}
			std::stringstream msg;
			msg << "scancode: " << (key.pressed ? "d " : "u ")
					<< (uint32_t) key.scancode << ", ctrl: " << key.ctrl
					<< ", alt: " << key.alt << ", shift: " << key.shift
					<< std::endl;
			screen->write(msg.str());
		}
		return true;

	} else if (command == BUILTIN_COMMAND_KBD_INFO) {
		screen->write(
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
		screen->write(msg.str());
		return true;

	} else if (command.substr(0,
			std::string(BUILTIN_COMMAND_CD).length()) == BUILTIN_COMMAND_CD) {

		working_directory = command.substr(
				std::string(BUILTIN_COMMAND_CD).length());
		g_set_working_directory_status stat = write_working_directory();

		if (stat == G_SET_WORKING_DIRECTORY_NOT_A_FOLDER) {
			screen->write("not a folder\n");
		} else if (stat == G_SET_WORKING_DIRECTORY_NOT_FOUND) {
			screen->write("not found\n");
		} else if (stat == G_SET_WORKING_DIRECTORY_ERROR) {
			screen->write("failed to switch\n");
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
			screen->write(msg.str());
			g_sleep(ms);
			screen->write("awake!\n");
		} else {
			std::stringstream msg;
			msg << "can't sleep this long (";
			msg << ms;
			msg << "ms)\n";
			screen->write(msg.str());
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
		screen->write(msg.str());
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
			screen->write(msg.str());
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

	int buflen = 512;
	char* buf = new char[buflen];

	while (!data->stop) {
		g_fs_read_status stat;
		int r = g_read_s(data->stdout_read_end, buf, buflen, &stat);

		if (stat == G_FS_READ_SUCCESSFUL) {
			std::stringstream o;
			for (int i = 0; i < r; i++) {
				char c = buf[i];
				if (c == '\r') {
					continue;
				} else if (c == '\t') {
					o << "    ";
				} else {
					o << c;
				}
			}

			uint8_t color = (data->err ? SC_ERROR_COLOR : SC_DEFAULT_COLOR);
			data->screen->write(o.str(), color);
			data->screen->updateCursor();
		}
	}
	data->screen->updateCursor();

	delete buf;
}

/**
 *
 */
void terminal_t::standard_in_thread(standard_in_thread_data_t* data) {

	std::string line;

	while (!data->stop) {
		terminal_input_status_t stat;

		// we add up the line because it might contain content already
		line = terminal_t::read_input(line, data->terminal->screen, &stat,
				&data->stop);

		if (stat == TERMINAL_INPUT_STATUS_EXIT) {
			if (data->int_pid != -1) {
				g_raise_signal(data->int_pid, SIGINT);
			}

		} else if (stat == TERMINAL_INPUT_STATUS_SCREEN_SWITCH) {
			data->terminal->switch_to_next();
			g_atomic_block(&data->terminal->inactive);

		} else if (stat == TERMINAL_INPUT_STATUS_DEFAULT) {
			data->terminal->screen->write("\n");
			data->terminal->screen->updateCursor();
			line += "\n";

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
bool terminal_t::find_in_path(std::string path, std::string& out) {

	std::string in_cwd = working_directory + "/" + path;
	if (file_exists(in_cwd)) {
		out = in_cwd;
		return true;
	}

	if (file_exists(path)) {
		out = path;
		return true;
	}

	std::string in_apps = "/applications/" + path;
	if (file_exists(in_apps)) {
		out = in_apps;
		return true;
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
	if (!find_in_path(shortpath, realpath)) {
		screen->write(shortpath + ": command not found\n");
		return false;
	}

	// spawn binary
	g_spawn_status status = g_spawn_poi(realpath.c_str(), args.c_str(),
			working_directory.c_str(),
			G_SECURITY_LEVEL_APPLICATION, out_pid, out_stdio, in_stdio);

	if (status == G_SPAWN_STATUS_SUCCESSFUL) {
		return true;
	}

	std::stringstream msg;
	msg << "failed to execute binary '";
	msg << realpath;
	msg << "', code: ";
	msg << status;
	msg << "\n";
	screen->write(msg.str(), SC_ERROR_COLOR);
	return false;
}

/**
 *
 */
std::string terminal_t::read_input(std::string there, screen_t* screen,
		terminal_input_status_t* out_status, bool* do_break) {

	std::string line = there;
	while (!*do_break) {
		g_key_info key = g_keyboard::readKey(do_break);

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

			} else {
				char chr = g_keyboard::charForKey(key);
				if (chr != -1) {
					line += chr;

					std::string out;
					out += chr;
					screen->write(out);
				}
			}

			screen->updateCursor();
		}
	}

	return line;
}
