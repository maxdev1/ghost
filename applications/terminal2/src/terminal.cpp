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
#include <gui_screen/gui_screen.hpp>
#include <unistd.h>
#include <ghostuser/io/terminal.hpp>
#include <signal.h>

/**
 * Main routine that initializes the terminal. The terminal can run in headless
 * and graphical mode - this is determined by the respective command line
 * argument.
 */
int main(int argc, char* argv[]) {

	// process command line arguments
	bool headless = false;

	for (int i = 0; i < argc; i++) {
		if (strcmp("--headless", argv[i]) == 0) {
			headless = true;
		}
	}

	terminal_t term(headless);
	term.execute();
}

/**
 * Executes the terminal instance. This initializes the screen, laods the
 * keyboard layout for this process and then starts the actual shell process.
 * Then control is given to the routines that process user input and program
 * output.
 */
void terminal_t::execute() {

	// initialize the screen
	initializeScreen();
	if (!screen) {
		klog("Terminal: Failed to initialize screen");
		return;
	}
	screen->clean();

	// load keyboard layout
	std::string initialLayout = "de-DE";
	if (!g_keyboard::loadLayout(initialLayout)) {
		g_logger::log("Terminal: Failed to load keyboard layout: " + initialLayout);
		return;
	}

	// disable video logging
	g_set_video_log(false);

	// start shell
	start_shell();
	if (shell_in == G_FD_NONE || shell_out == G_FD_NONE || shell_err == G_FD_NONE) {
		return;
	}

	// start in/out routines
	output_routine_startinfo_t* out_info = new output_routine_startinfo_t();
	out_info->error_output = false;
	out_info->terminal = this;
	g_create_thread_d((void*) &output_routine, out_info);

	output_routine_startinfo_t* err_info = new output_routine_startinfo_t();
	err_info->error_output = true;
	err_info->terminal = this;
	g_create_thread_d((void*) &output_routine, err_info);

	input_routine();
}

/**
 * Initializes either the graphical window screen or a headless screen (that
 * writes to the BIOS video memory area).
 */
void terminal_t::initializeScreen() {

	if (headless) {
		if (g_task_get_id("terminal_headless") != -1) {
			fprintf(stderr, "Terminal: Can only be executed once when in headless mode");
			return;
		}
		g_task_register_id("terminal_headless");

		screen = new headless_screen_t();
		return;
	}

	gui_screen_t* gui_screen = new gui_screen_t();
	if (gui_screen->initialize()) {
		screen = gui_screen;
	} else {
		fprintf(stderr, "Terminal: Failed to initialize the graphical screen");
	}
}

/**
 * Starts the shell executable and passes pipes for their input, output and
 * error streams.
 */
void terminal_t::start_shell() {

	// create input & output pipes
	g_fs_pipe_status stdin_stat;
	g_fd shellin_w;
	g_fd shellin_r;
	g_pipe_s(&shellin_w, &shellin_r, &stdin_stat);
	if (stdin_stat != G_FS_PIPE_SUCCESSFUL) {
		klog("Terminal: Failed to setup stdin pipe for shell");
		return;
	}

	g_fs_pipe_status stdout_stat;
	g_fd shellout_w;
	g_fd shellout_r;
	g_pipe_s(&shellout_w, &shellout_r, &stdout_stat);
	if (stdout_stat != G_FS_PIPE_SUCCESSFUL) {
		klog("Terminal: Failed to setup stdout pipe for shell");
		return;
	}

	g_fs_pipe_status stderr_stat;
	g_fd shellerr_w;
	g_fd shellerr_r;
	g_pipe_s(&shellerr_w, &shellerr_r, &stderr_stat);
	if (stderr_stat != G_FS_PIPE_SUCCESSFUL) {
		klog("Terminal: Failed to setup stderr pipe for shell");
		return;
	}

	// spawn binary
	g_pid out_pid;
	g_fd stdio_in[3];
	stdio_in[0] = shellin_r;
	stdio_in[1] = shellout_w;
	stdio_in[2] = shellerr_w;
	g_fd stdio_target[3];
	g_spawn_status status = g_spawn_poi("/applications/gosh.bin", "", "/", G_SECURITY_LEVEL_APPLICATION, &out_pid, stdio_target, stdio_in);

	if (status != G_SPAWN_STATUS_SUCCESSFUL) {
		klog("Terminal: Failed to spawn shell process");
		return;
	}

	shell_in = shellin_w;
	shell_out = shellout_r;
	shell_err = shellerr_r;
}

/**
 * Simple utility function to write a C++ string to the shell executables input.
 */
void terminal_t::write_string_to_shell(std::string line) {

	const char* lineContent = line.c_str();
	int lineLength = strlen(lineContent);

	int written = 0;
	int len = 0;
	while (written < lineLength) {
		len = write(shell_in, &lineContent[written], lineLength - written);
		if (len <= 0) {
			break;
		}
		written += len;
	}
}

/**
 * Sends a terminal key to the shell. These keys are encoded in a specific way
 * so that the programs can receive more than simple text characters.
 */
void terminal_t::write_termkey_to_shell(int termkey) {

	char buf[3];
	buf[0] = G_TERMKEY_SUB;
	buf[1] = termkey & 0xFF;
	buf[2] = (termkey >> 8) & 0xFF;
	write(shell_in, &buf, 3);
}

/**
 * Main input routine that receives typed keys by the user and sends them to the
 * shell executable respectively.
 */
void terminal_t::input_routine() {

	std::string buffer = "";
	while (true) {
		g_key_info readInput = screen->readInput();

		// Default line-buffered input
		if (input_mode == G_TERMINAL_MODE_DEFAULT) {

			if (readInput.key == "KEY_ENTER" && readInput.pressed) {
				if (echo) {
					screen_lock.lock();
					screen->writeChar('\n');
					screen_lock.unlock();
				}

				buffer += '\n';
				write_string_to_shell(buffer);

				buffer = "";

			} else if ((readInput.ctrl && readInput.key == "KEY_C") || (readInput.key == "KEY_ESC")) {
				if (current_process) {
					g_raise_signal(current_process, SIGINT);
				}

			} else if (readInput.key == "KEY_BACKSPACE" && readInput.pressed) {
				buffer = buffer.size() > 0 ? buffer.substr(0, buffer.size() - 1) : buffer;
				screen->backspace();

			} else {
				char chr = g_keyboard::charForKey(readInput);
				if (chr != -1) {
					buffer += chr;

					if (echo) {
						screen_lock.lock();
						screen->writeChar(chr);
						screen_lock.unlock();
					}
				}
			}

		} else if (input_mode == G_TERMINAL_MODE_RAW) {

			if (readInput.key == "KEY_ENTER" && readInput.pressed) {
				write_termkey_to_shell(G_TERMKEY_ENTER);

				if (echo) {
					screen_lock.lock();
					screen->writeChar('\n');
					screen_lock.unlock();
				}

			} else if (readInput.key == "KEY_BACKSPACE" && readInput.pressed) {
				write_termkey_to_shell(G_TERMKEY_BACKSPACE);

				if (echo) {
					screen_lock.lock();
					screen->backspace();
					screen_lock.unlock();
				}

			} else if (readInput.key == "KEY_ARROW_LEFT" && readInput.pressed) {
				write_termkey_to_shell(G_TERMKEY_LEFT);

			} else if (readInput.key == "KEY_ARROW_RIGHT" && readInput.pressed) {
				write_termkey_to_shell(G_TERMKEY_RIGHT);

			} else if (readInput.key == "KEY_ARROW_UP" && readInput.pressed) {
				write_termkey_to_shell(G_TERMKEY_UP);

			} else if (readInput.key == "KEY_ARROW_DOWN" && readInput.pressed) {
				write_termkey_to_shell(G_TERMKEY_DOWN);

			} else if (readInput.key == "KEY_TAB" && readInput.pressed && readInput.shift) {
				write_string_to_shell("\t");

			} else if (readInput.key == "KEY_TAB" && readInput.pressed) {
				write_termkey_to_shell(G_TERMKEY_STAB);

			} else {
				char chr = g_keyboard::charForKey(readInput);
				if (chr != -1) {
					write(shell_in, &chr, 1);

					if (echo) {
						screen_lock.lock();
						screen->writeChar(chr);
						screen_lock.unlock();
					}
				}
			}
		}
	}
}

/**
 *
 */
void terminal_t::output_routine(output_routine_startinfo_t* info) {

	int buflen = 1024;
	char* buf = new char[buflen];

	stream_control_status_t status;

	while (true) {
		g_fs_read_status stat;
		int r = g_read_s(info->error_output ? info->terminal->shell_err : info->terminal->shell_out, buf, buflen, &stat);

		if (stat == G_FS_READ_SUCCESSFUL) {

			for (int i = 0; i < r; i++) {
				char c = buf[i];

				// Lock screen and set error color if required
				info->terminal->screen_lock.lock();
				info->terminal->process_output_character(&status, info->error_output, c);
				info->terminal->screen_lock.unlock();
			}
		} else {
			break;
		}
	}

	// clean up
	delete buf;
	delete info;
}

/**
 *
 */
void terminal_t::process_output_character(stream_control_status_t* status, bool error_stream, char c) {

	if (status->status == TERMINAL_STREAM_STATUS_TEXT) {

		// Simple textual output
		if (c == '\r') {
			return;

		} else if (c == '\t') {
			screen->writeChar(' ');
			screen->writeChar(' ');
			screen->writeChar(' ');
			screen->writeChar(' ');

		} else if (c == 27 /* ESC */) {
			status->status = TERMINAL_STREAM_STATUS_LAST_WAS_ESC;

		} else {
			int fg = screen->getColorForeground();
			if (error_stream) {
				screen->setColorForeground(SC_RED);
			}
			screen->writeChar(c);
			if (error_stream) {
				screen->setColorForeground(fg);
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
	} else if (status->status == TERMINAL_STREAM_STATUS_WITHIN_VT100 || status->status == TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM) {

		// Parameter value
		if (c >= '0' && c <= '9') {
			if (status->parameterCount == 0) {
				status->parameterCount = 1;
			}

			if (status->parameterCount <= TERMINAL_STREAM_CONTROL_MAX_PARAMETERS) {
				status->parameters[status->parameterCount - 1] = status->parameters[status->parameterCount - 1] * 10;
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
				process_vt100_sequence(status);

			} else if (status->status == TERMINAL_STREAM_STATUS_WITHIN_GHOSTTERM) {
				process_ghostterm_sequence(status);
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
void terminal_t::process_vt100_sequence(stream_control_status_t* status) {

	switch (status->controlCharacter) {
	// Cursor up
	case 'A':
		screen->moveCursor(screen->getCursorX(), screen->getCursorY() - status->parameters[0]);
		break;

		// Cursor down
	case 'B':
		screen->moveCursor(screen->getCursorX(), screen->getCursorY() + status->parameters[0]);
		break;

		// Cursor forward
	case 'C':
		screen->moveCursor(screen->getCursorX() + status->parameters[0], screen->getCursorY());
		break;

		// Cursor back
	case 'D':
		screen->moveCursor(screen->getCursorX() - status->parameters[0], screen->getCursorY());
		break;

		// Mode setting
	case 'm':
		for (int i = 0; i < status->parameterCount; i++) {
			int param = status->parameters[i];

			// Reset
			if (param == 0) {
				screen->setColorBackground(SC_BLACK);
				screen->setColorForeground(SC_WHITE);

				// Foreground color
			} else if (param >= 30 && param < 40) {
				screen->setColorForeground(convert_vt100_to_screen_color(param - 30));

				// Background color
			} else if (param >= 40 && param < 50) {
				screen->setColorBackground(convert_vt100_to_screen_color(param - 40));

			}
		}

		break;

		// Clearing
	case 'J':
		if (status->parameterCount == 1) {
			// Clear the entire screen
			if (status->parameters[0] == 2) {
				screen->clean();
			}
		}
		break;

		// Reposition cursor
	case 'f':
		screen->moveCursor(status->parameters[1], status->parameters[0]);
		break;

		// Cursor queries
	case 'n':
		// Query position
		if (status->parameters[0] == 6) {
			std::stringstream response;
			response << (char) G_TERMKEY_ESC << "[" << screen->getCursorY() << ";" << screen->getCursorX() << "R";
			auto responseStr = response.str();
			write(shell_in, responseStr.c_str(), responseStr.size());
		}
		break;

		// Set scroll area
	case 'r':
		if(status->parameterCount == 0) {
			screen->setScrollAreaScreen();
		} else {
			screen->setScrollArea(status->parameters[0], status->parameters[1]);
		}
		break;

		// Scroll Scrolling Region Up
	case 'S':
		screen->scroll(status->parameters[0]);
		break;

		// Scroll Scrolling Region Down
	case 'T':
		screen->scroll(-status->parameters[0]);
		break;
	}
}

/**
 *
 */
void terminal_t::process_ghostterm_sequence(stream_control_status_t* status) {

	switch (status->controlCharacter) {

	// Change mode
	case 'm':
		this->input_mode = (g_terminal_mode) status->parameters[0];
		break;

		// Change echo
	case 'e':
		this->echo = (status->parameters[0] == 1);
		break;

		// Screen info
	case 'i': {
		std::stringstream response;
		response << (char) G_TERMKEY_ESC << "{" << screen->getWidth() << ";" << screen->getHeight() << "i";
		write_string_to_shell(response.str().c_str());
		break;
	}

		// Put char
	case 'p':
		screen->writeChar(status->parameters[0]);
		break;

		// Process control
	case 'c':
		current_process = status->parameters[0];
		break;

		// Various other controls
	case 'C':
		// show/hide cursor
		if (status->parameters[0] == 0) {
			screen->setCursorVisible(status->parameters[1]);
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
	case VT100_COLOR_GRAY:
		return SC_LGRAY;
	}
	return SC_WHITE;
}

