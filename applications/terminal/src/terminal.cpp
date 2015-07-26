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

#include <Terminal.hpp>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>

#include <ghost.h>
#include <ghostuser/utils/Logger.hpp>
#include <ghostuser/io/Keyboard.hpp>

static std::vector<terminal_t*> terminals;
static std::vector<g_tid> terminal_threads;
static terminal_t* active_terminal = 0;

/**
 *
 */
int main(int argc, char* argv[]) {

	if (g_task_get_id("terminal") != -1) {
		std::cout << "error: can't execute terminal process twice" << std::endl;
	}

	terminal_t::prepare();
	terminal_t::create_terminal();

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
void terminal_t::create_terminal() {
	g_tid term = g_create_thread((void*) &terminal_t::add_terminal);
	terminal_threads.push_back(term);
}

/**
 *
 */
void terminal_t::add_terminal() {
	terminal_t* terminal = new terminal_t();
	terminals.push_back(terminal);

	if (active_terminal) {
		terminal->working_directory = active_terminal->working_directory;
	}

	switch_to(terminal);
	terminal->run();
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
void terminal_t::run() {

	read_working_directory();
	screen->clean();
	screen->activate();
	screen->write("Welcome to Ghost! :)\n");
	screen->write(
			std::string("Enter '") + BUILTIN_COMMAND_HELP
					+ "' for a list of commands.\n\n");

	std::string command;

	bool write_header = true;
	while (true) {
		// wait for activity
		g_atomic_block(&inactive);
		screen->updateCursor();

		if (write_header) {
			// write current directory
			screen->write(working_directory);

			screen->write(">");
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
			create_terminal();
			write_header = false;

		} else if (status == TERMINAL_INPUT_STATUS_EXIT) {
			write_header = false;

		} else if (status == TERMINAL_INPUT_STATUS_DEFAULT) {
			screen->write("\n");

			if (!command.empty()) {
				if (!handle_builtin(command)) {
					// TODO parse arguments
					std::string path = command;
					std::string args = "";

					int spacePos = path.find(' ');
					if (spacePos != -1) {
						args = path.substr(spacePos + 1);
						path = path.substr(0, spacePos);
					}

					execute(path, args, false);
				}
			}

			command = "";
		}
	}
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

	} else if (command == BUILTIN_COMMAND_LS) {
		g_fs_open_directory_status stat;
		auto* iter = g_open_directory_s(working_directory.c_str(), &stat);

		if (stat == G_FS_OPEN_DIRECTORY_SUCCESSFUL) {
			g_fs_directory_entry* entry;

			while (true) {
				g_fs_read_directory_status rstat;
				entry = g_read_directory_s(iter, &rstat);

				if (rstat == G_FS_READ_DIRECTORY_SUCCESSFUL) {
					if (entry == 0) {
						break;
					}

					if (entry->type == G_FS_NODE_TYPE_FILE) {
						screen->write("   ");
					} else if (entry->type == G_FS_NODE_TYPE_FOLDER) {
						screen->write(" + ");
					} else {
						screen->write(" ~ ");
					}
					screen->write(entry->name);
					screen->write("\n");
				} else {
					break;
				}
			}

			g_close_directory(iter);
		} else {
			screen->write("failed to read directory\n");
		}
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

	} else if (command == BUILTIN_COMMAND_KBD_INFO) {
		screen->write(
				"keyboard layout is '" + g_keyboard::getCurrentLayout()
						+ "'\n");
		return true;

	} else if (command.substr(0,
			std::string(BUILTIN_COMMAND_READ).length()) == BUILTIN_COMMAND_READ) {
		command = command.substr(std::string(BUILTIN_COMMAND_READ).length());

		FILE* file = fopen(command.c_str(), "r");

		if (file == NULL) {
			screen->write("file '" + command + "' not found\n");
		} else {
			bool end = false;
			int lines = 0;
			while (true) {
				char c = fgetc(file);
				if (c == '\n' && !end) {

					if (lines == 8) {
						lines = 0;
						while (true) {
							auto info = g_keyboard::readKey();
							if (info.key == "KEY_ESC") {
								end = true;
								break;
							}
							if (info.pressed) {
								break;
							}
						}
					}
				}

				if (isprint(c) || c == '\n') {
					screen->write(std::string("") + c);
				} else if (c == '\t') {
					screen->write("    ");
				}
				if (feof(file)) {
					break;
				}
			}

			fclose(file);
		}
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

		execute(path, args, true);
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
		create_terminal();
		return true;

	}

	return false;
}

/**
 *
 */
void terminal_t::standard_out_thread(standard_out_thread_data_t* data) {

	int buflen = 512;
	char buf[buflen];

	while (!data->stop) {
		g_fs_read_status stat;
		int r = g_read_s(data->stdout_read_end, buf, buflen, &stat);

		std::stringstream o;
		for (int i = 0; i < r; i++) {
			o << buf[i];
		}
		data->screen->write(o.str());
		data->screen->updateCursor();
	}
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
			g_raise_signal(data->pid, SIGINT);

		} else if (stat == TERMINAL_INPUT_STATUS_SCREEN_SWITCH) {
			data->terminal->switch_to_next();
			g_atomic_block(&data->terminal->inactive);

		} else if (stat == TERMINAL_INPUT_STATUS_DEFAULT) {
			data->terminal->screen->write("\n");
			line += "\n";
			const char* lineContent = line.c_str();
			int lineLength = strlen(lineContent);

			int written = 0;
			int len = 0;
			while (written < lineLength) {
				len = g_write(data->stdin_write_end, &lineContent[written],
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

	std::string in_apps = "/ramdisk/applications/" + path;
	if (file_exists(in_apps)) {
		out = in_apps;
		return true;
	}

	return false;
}

/**
 *
 */
void terminal_t::execute(std::string shortpath, std::string args,
		bool background) {

	// spawn the process
	g_pid process_id;
	g_fd process_io[2];

	// check for command in path
	std::string realpath;
	if (!find_in_path(shortpath, realpath)) {
		screen->write("command not found\n");
		return;
	}

	// spawn binary
	g_spawn_status status = g_spawn(realpath.c_str(), args.c_str(),
	G_SECURITY_LEVEL_APPLICATION, &process_id, process_io);

	if (status == G_SPAWN_STATUS_SUCCESSFUL) {

		if (background) {
			std::stringstream msg;
			msg << "process ";
			msg << process_id;
			msg << "\n";
			screen->write(msg.str());

		} else {
			uint32_t terminal_pid = g_get_pid();

			// clone descriptors
			g_fd process_stdin = g_clone_fd(process_io[0], process_id,
					terminal_pid);
			g_fd process_stdout = g_clone_fd(process_io[1], process_id,
					terminal_pid);

			// create threads
			standard_out_thread_data_t out_data;
			out_data.stop = false;
			out_data.stdout_read_end = process_stdout;
			out_data.screen = screen;
			g_tid rout = g_create_thread_d((void*) &standard_out_thread,
					(void*) &out_data);

			standard_in_thread_data_t in_data;
			in_data.stop = false;
			in_data.stdin_write_end = process_stdin;
			in_data.pid = process_id;
			in_data.terminal = this;
			g_tid rin = g_create_thread_d((void*) &standard_in_thread,
					(void*) &in_data);

			// wait for process to exit
			g_join(process_id);
			in_data.stop = true;
			out_data.stop = true;

			screen->write("\n");

			// wait for input/output thread before leaving
			g_join(rout);
			g_join(rin);
		}

	} else {
		std::stringstream msg;
		msg << "failed to execute binary '";
		msg << realpath;
		msg << "', code: ";
		msg << status;
		msg << "\n";
		screen->write(msg.str());
	}
}

/**
 *
 */
std::string terminal_t::read_input(std::string there, screen_t* screen,
		terminal_input_status_t* out_status, bool* do_break) {

	std::string line = there;
	while (!*do_break) {
		g_key_info key = g_keyboard::readKey();

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
