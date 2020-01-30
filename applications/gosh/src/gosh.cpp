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

#include <iostream>
#include <ghostuser/io/terminal.hpp>
#include <ghostuser/utils/local.hpp>
#include <gosh.hpp>
#include <stdio.h>
#include <parser.hpp>

#include <string>
#include <string.h>
#include <sstream>

/**
 *
 */
bool readInputLine(std::string& line) {

	g_terminal::setMode(G_TERMINAL_MODE_RAW);
	g_terminal::setEcho(false);

	int caret = 0;

	while (true) {
		int c = g_terminal::getChar();
		if (c == -1) {
			return false;
		}

		if (c == G_TERMKEY_BACKSPACE) {
			if (line.size() > 0 && caret > 0) {
				char deleted = line.at(caret - 1);

				auto pos = g_terminal::getCursor();
				auto afterCaret = line.substr(caret);
				line = line.substr(0, caret - 1) + afterCaret;
				caret--;

				if (deleted == '\t') {
					pos.x -= 4;
					afterCaret += "    ";
				} else {
					pos.x--;
					afterCaret += ' ';
				}
				g_terminal::setCursor(pos);
				for (char c : afterCaret) {
					std::cout << c;
				}
				g_terminal::setCursor(pos);
			}

		} else if (c == '\t' || c == G_TERMKEY_STAB /* TODO implement completion */) {
			auto pos = g_terminal::getCursor();
			std::cout << "    ";

			auto afterCaret = line.substr(caret);
			line = line.substr(0, caret) + '\t' + afterCaret;
			caret++;

			pos.x += 4;
			g_terminal::setCursor(pos);
			for (char c : afterCaret) {
				std::cout << c;
			}
			g_terminal::setCursor(pos);

		} else if (c == G_TERMKEY_ENTER) {
			std::cout << '\n';
			break;

		} else if (c == G_TERMKEY_LEFT) {
			if (caret > 0) {
				char beforeCaret = line.at(caret - 1);

				caret--;
				if (beforeCaret == '\t') {
					g_terminal::moveCursorBack(4);
				} else {
					g_terminal::moveCursorBack(1);
				}
			}

		} else if (c == G_TERMKEY_RIGHT) {
			if (caret < line.size()) {
				char atCaret = line.at(caret);
				caret++;
				if (atCaret == '\t') {
					g_terminal::moveCursorForward(4);
				} else {
					g_terminal::moveCursorForward(1);
				}
			}

		} else if (c < 0x100) {
			auto pos = g_terminal::getCursor();
			std::cout << (char) c;

			auto afterCaret = line.substr(caret);
			line = line.substr(0, caret) + (char) c + afterCaret;
			caret++;

			pos.x++;
			g_terminal::setCursor(pos);
			for (char c : afterCaret) {
				std::cout << c;
			}
			g_terminal::setCursor(pos);
		}
	}

	return true;
}

/**
 *
 */
bool file_exists(std::string path) {

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
std::string find_program(std::string cwd, std::string name) {

	// check for match with cwd
	std::string path = cwd + "/" + name;
	if (file_exists(path)) {
		return path;
	}

	// check for full path
	if (file_exists(name)) {
		return name;
	}

	// check for /applications folder
	path = "/applications/" + name;
	if (file_exists(path)) {
		return path;
	}

	// last chance - check for .bin extension
	if (name.length() < 4 || name.substr(name.length() - 4) != ".bin") {
		return find_program(cwd, name + ".bin");
	}

	// nothing found
	return name;
}

/**
 *
 */
bool handle_builtin(std::string cwd, program_call_t* call) {

	if (call->program == "cd") {
		if (call->arguments.size() == 1) {
			auto newdir = call->arguments.at(0);
			g_set_working_directory(newdir.c_str());
		} else {
			std::cerr << "Usage:\tcd /path/to/target" << std::endl;
		}
		return true;
	}

	if (call->program == "clear") {
		g_terminal::clear();
		g_term_cursor_position pos;
		pos.x = 0;
		pos.y = 0;
		g_terminal::setCursor(pos);
		return true;
	}

	if (call->program == "which") {
		if (call->arguments.size() == 1) {
			std::cout << find_program(cwd, call->arguments.at(0)) << std::endl;

		} else {
			std::cerr << "Usage:\twhich command" << std::endl;
		}
		return true;
	}

	return false;
}

/**
 *
 */
int main(int argc, char *argv[]) {

	g_terminal::setCursor(g_term_cursor_position(0, 0));

	// check if we're running headless
	bool running_headless = (g_task_get_id("terminal_headless") != -1);
	if (running_headless) {
		std::cout
				<< "Enter 'read README' for a quick introduction. Use 'launch ui' to run the GUI."
				<< std::endl;
	}
	char* cwdbuf = new char[G_PATH_MAX];

	while (true) {

		// print cwd
		std::cout << (char) 27 << "[38m";
		if (g_get_working_directory(cwdbuf)
				== G_GET_WORKING_DIRECTORY_SUCCESSFUL) {
			std::cout << cwdbuf;
		} else {
			std::cout << "?";
		}
		std::string cwd(cwdbuf);
		std::cout << '>';
		std::cout << (char) 27 << "[0m";
		std::flush(std::cout);

		g_terminal::setCursor(g_terminal::getCursor());

		std::string line;
		if (!readInputLine(line)) {
			break;
		}

		// switch to normal input mode
		g_terminal::setMode(G_TERMINAL_MODE_DEFAULT);
		g_terminal::setEcho(true);

		parser_t cmdparser(line);
		pipe_expression_t* pipeexpr;
		if (!cmdparser.pipe_expression(&pipeexpr)) {
			continue;
		}

		// perform spawning
		g_fd previous_out_pipe_r = -1;
		g_pid first_process_id = -1;
		g_pid last_process_id = -1;
		bool success = false;

		auto num_calls = pipeexpr->calls.size();
		for (int c = 0; c < num_calls; c++) {
			program_call_t* call = pipeexpr->calls[c];

			// builtin calls (only allowed as single call)
			if (num_calls == 1 && c == 0 && handle_builtin(cwd, call)) {
				break;
			}

			// concatenate arguments to one argument string
			std::stringstream argstream;
			bool first = true;
			for (auto arg : call->arguments) {
				if (first) {
					first = false;
				} else {
					argstream << (char) G_CLIARGS_SEPARATOR;
				}
				argstream << arg;
			}

			// create out pipe if necessary
			g_fd out_pipe_w;
			g_fd out_pipe_r;
			if (num_calls > 1 && c < num_calls - 1) {
				g_fs_pipe_status pipe_stat = g_pipe(&out_pipe_w, &out_pipe_r);

				if (pipe_stat != G_FS_PIPE_SUCCESSFUL) {
					std::cerr << "failed to create output pipe when spawning '"
							<< call->program << "'" << std::endl;
					// TODO clean up pipes?
					success = false;
					break;
				}
			}
			// decide how to set in/out/err file descriptors
			g_fd in_stdio[3];

			// stderr is always the same
			in_stdio[2] = STDERR_FILENO;

			// stdin must be chosen
			if (c == 0) {
				in_stdio[0] = STDIN_FILENO;
			} else {
				in_stdio[0] = previous_out_pipe_r;
			}

			// stdout must be chosen
			if ((num_calls == 1 && c == 0) || c == num_calls - 1) {
				in_stdio[1] = STDOUT_FILENO;
			} else {
				in_stdio[1] = out_pipe_w;
			}

			// do spawning
			g_pid out_pid;
			g_fd out_stdio[3];
			g_spawn_status status = g_spawn_poi(
					find_program(cwd, call->program).c_str(),
					argstream.str().c_str(), cwdbuf,
					G_SECURITY_LEVEL_APPLICATION, &out_pid, out_stdio,
					in_stdio);

			// check result
			if (status == G_SPAWN_STATUS_SUCCESSFUL) {
				if (first_process_id == -1) {
					first_process_id = out_pid;
				}
				last_process_id = out_pid;
				success = true;

				// close write end in this process
				g_close(out_pipe_w);

				if (previous_out_pipe_r != -1) {
					// close read end of previous pipe in this process
					g_close(previous_out_pipe_r);
				}

				// remember for next process
				previous_out_pipe_r = out_pipe_r;

			} else {
				success = false;
				// error during one spawn
				// TODO clean up pipes
				std::cout << (char) 27 << "[31m";
				if (status == G_SPAWN_STATUS_FORMAT_ERROR) {
					std::cout << call->program << ": invalid binary format"
							<< std::endl;
				} else if (status == G_SPAWN_STATUS_IO_ERROR) {
					std::cout << call->program << ": command not found"
							<< std::endl;
				} else {
					std::cout << call->program
							<< ": unknown error during program execution"
							<< std::endl;
				}
				std::cout << (char) 27 << "[0m";
				std::flush(std::cout);
				break;
			}
		}

		if (success) {
			// join into the last process
			g_terminal::setControlProcess(last_process_id);
			g_join(last_process_id);
			g_terminal::setControlProcess(0);
		}
	}

	delete cwdbuf;
}
