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
#include <gosh.hpp>
#include <stdio.h>

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
				auto pos = g_terminal::getCursor();
				auto afterCaret = line.substr(caret);
				line = line.substr(0, caret - 1) + afterCaret;
				caret--;

				afterCaret += ' ';
				pos.x--;
				g_terminal::setCursor(pos);
				for (char c : afterCaret) {
					g_terminal::putChar(c);
				}
				g_terminal::setCursor(pos);
			}

		} else if (c == G_TERMKEY_ENTER) {
			g_terminal::putChar('\n');
			break;

		} else if (c == G_TERMKEY_LEFT) {
			if (caret > 0) {
				caret--;
				g_terminal::moveCursorBack(1);
			}

		} else if (c == G_TERMKEY_RIGHT) {
			if (caret < line.size()) {
				caret++;
				g_terminal::moveCursorForward(1);
			}

		} else if (c < 0x100) {
			auto pos = g_terminal::getCursor();
			g_terminal::putChar(c);

			auto afterCaret = line.substr(caret);
			line = line.substr(0, caret) + (char) c + afterCaret;
			caret++;

			pos.x++;
			g_terminal::setCursor(pos);
			for (char c : afterCaret) {
				g_terminal::putChar(c);
			}
			g_terminal::setCursor(pos);
		}
	}

	return true;
}

/**
 *
 */
int main(int argc, char *argv[]) {

	g_terminal::setCursor(g_term_cursor_position(0, 0));

	char* cwdbuf = new char[G_PATH_MAX];

	while (true) {

		// print cwd
		if (g_get_working_directory(cwdbuf)
				== G_GET_WORKING_DIRECTORY_SUCCESSFUL) {
			std::cout << cwdbuf;
		} else {
			std::cout << "?";
		}

		std::cout << '>';
		std::flush(std::cout);

		std::string line;
		if (!readInputLine(line)) {
			break;
		}

		// switch to normal input mode
		g_terminal::setMode(G_TERMINAL_MODE_DEFAULT);
		g_terminal::setEcho(true);

		// spawn binary
		g_fd in_stdio[3];
		in_stdio[0] = STDIN_FILENO;
		in_stdio[1] = STDOUT_FILENO;
		in_stdio[2] = STDERR_FILENO;

		g_pid out_pid;
		g_fd out_stdio[3];
		g_spawn_status status = g_spawn_poi(line.c_str(), "", "/",
		G_SECURITY_LEVEL_APPLICATION, &out_pid, out_stdio, in_stdio);

		// check result
		if (status == G_SPAWN_STATUS_SUCCESSFUL) {
			g_terminal::setControlProcess(out_pid);
			g_join(out_pid);
			g_terminal::setControlProcess(-1);

		} else {
			std::cout << (char) 27 << "[31m";
			if (status == G_SPAWN_STATUS_FORMAT_ERROR) {
				std::cout << "Format error" << std::endl;
			} else if (status == G_SPAWN_STATUS_IO_ERROR) {
				std::cout << "I/O error" << std::endl;
			} else {
				std::cout << "Unknown error" << std::endl;
			}
			std::cout << (char) 27 << "[0m";
			std::flush(std::cout);
		}
	}

	delete cwdbuf;
}
