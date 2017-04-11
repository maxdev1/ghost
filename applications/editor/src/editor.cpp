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

#include <editor.hpp>
#include <iostream>
#include <ghostuser/io/terminal.hpp>
#include <stdio.h>
#include <unistd.h>
#include <sstream>
#include <string.h>

#define IS_PRINTED_CHAR(c)		(c != '\r')

// local variables
int scroll_line;
int caret_line;
int caret_col;
FILE* file;

g_term_dimension terminal_size;
uint8_t* buffer;
uint32_t buffer_size;
uint32_t buffer_bytes;

/**
 *
 */
int main(int argc, const char** argv) {

	// check arguments
	if (argc != 2) {
		std::cerr << "Usage:\t" << argv[0] << " filename" << std::endl;
		return -1;
	}

	// open input file
	file = fopen(argv[1], "r+");
	if (file == NULL) {
		std::cerr << argv[1] << ": failed to open file" << std::endl;
		return -1;
	}

	// get terminal information
	terminal_size = g_terminal::getSize();
	if (terminal_size.w == -1 || terminal_size.h == -1) {
		std::cerr << "failed to retrieve size of terminal" << std::endl;
		return -1;
	}

	// initialize buffer
	buffer_size = 1024;
	buffer = new uint8_t[buffer_size];

	// load first lines of file
	load_file();
	display_current_lines();
	g_terminal::setCursor(g_term_cursor_position(0, 0));
	scroll_line = 0;
	caret_line = 0;
	caret_col = 0;

	// enable raw mode
	g_terminal::setMode(G_TERMINAL_MODE_RAW);
	g_terminal::setEcho(false);

	while (true) {
		int c = g_terminal::getChar();

		if (c == G_TERMKEY_RIGHT) {
			/*if (buffer[caret] == '\r' && caret < buffer_bytes) {
			 caret++;
			 }
			 if (caret < buffer_bytes) {
			 caret++;
			 }*/

		} else if (c == G_TERMKEY_LEFT) {
			/*if (buffer[caret] == '\r' && caret > 0) {
			 caret--;
			 }
			 if (caret > 0) {
			 caret--;
			 }*/

		} else if (c == G_TERMKEY_STAB) {
			break;

		} else if (c == G_TERMKEY_BACKSPACE) {
			do_backspace();

		} else if (c == G_TERMKEY_ENTER) {
			/*insert_into_buffer((char*) "\n", 1);
			 caret++;*/

			// data input
		} else if (c < G_TERMKEY_MIN) {
			char cs = (char) c;
			insert_into_buffer(&cs, 1);
		}
	}
}

/**
 *
 */
void do_backspace() {

	/*if (caret == 0) {
	 return;
	 }

	 caret--;
	 memcpy(&buffer[caret], &buffer[caret + 1], buffer_size - caret);
	 buffer_size--;
	 buffer[buffer_size - 1] = 0;
	 display_current_lines();
	 update_visual_cursor();*/
}

/**
 *
 */
int get_caret_index() {
	uint8_t* ls = find_line(caret_line);
	return (int) (ls - buffer) + caret_col;
}

/**
 *
 */
int get_line_visual_length(int line) {
	uint8_t* ls = find_line(line);
	uint8_t* start = ls;

	int l = 0;
	while (*start && *start != '\n') {
		if (IS_PRINTED_CHAR(*start)) {
			l++;
		}
		start++;
	}
	return l;
}

/**
 *
 */
g_term_cursor_position to_visual(int line, int col) {

	uint8_t* start = find_line(scroll_line);

	int data_line = scroll_line;
	int data_col = 0;
	int vis_x = 0;
	int vis_y = 0;

	while (*start) {

		if (IS_PRINTED_CHAR(*start)) {
			if (line == data_line && col == data_col) {
				return g_term_cursor_position(vis_x, vis_y);
			}

			if (*start == '\n') {
				vis_x = 0;
				vis_y++;
				data_col = 0;
				data_line++;

			} else {
				vis_x++;
				data_col++;
			}

			if (vis_x > terminal_size.w) {
				vis_y++;
				vis_x = 0;
			}
		}

		start++;
	}

	return g_term_cursor_position(0, 0);
}

/**
 *
 */
void insert_into_buffer(char* source, uint32_t len) {

	// extend buffer if necessary
	while (buffer_size < buffer_bytes + len) {
		extend_buffer();
	}

	int caret = get_caret_index();
	int line_len = get_line_visual_length(caret_line);

	// put data in actual buffer
	for (int i = buffer_bytes - 1; i >= caret; i--) {
		if (i - len >= 0) {
			buffer[i] = buffer[i - len];
		}
	}
	memcpy(&buffer[caret], source, len);
	buffer_bytes += len;

	// scroll rest if line is now longer
	int height_before = line_len / terminal_size.w + 1;
	int height_after = (line_len + len) / terminal_size.w + 1;

	if (height_after > height_before) {
		g_terminal::setScrollArea(to_visual(caret_line + 1, caret_col).y, terminal_size.h - 1);
		g_terminal::scroll(height_after - height_before);
	}

	// print line
	g_terminal::setCursorVisible(false);
	g_terminal::setCursor(g_term_cursor_position(0, caret_line - scroll_line));

	uint8_t* start = find_line(caret_line);
	int printedChars = 0;
	while (*start && *start != '\n') {
		if (IS_PRINTED_CHAR(*start)) {
			std::cout << *start;
			++printedChars;
		}
		start++;
	}
	if (printedChars % terminal_size.w == 0) {
		printedChars++;
	}
	while (printedChars % terminal_size.w != 0) {
		std::cout << ' ';
		++printedChars;
	}
	std::flush(std::cout);

	// advance caret
	caret_col += len;

	// update visual caret
	g_terminal::setCursor(to_visual(caret_line, caret_col));
	g_terminal::setCursorVisible(true);
}

/**
 *
 */
void quit_with_error(std::string str) {

	g_terminal::setCursor(g_term_cursor_position(0, 0));
	std::cerr << str << std::endl;
	g_terminal::getChar();
	g_exit(-1);
}

/**
 *
 */
void extend_buffer() {

	buffer_size = buffer_size * 2;
	uint8_t* newbuf = new uint8_t[buffer_size];
	memcpy((void*) newbuf, (void*) buffer, buffer_bytes);
	delete buffer;
	buffer = newbuf;
}

/**
 *
 */
void load_file() {

	// fill buffer with data from file
	buffer_bytes = 0;
	while (true) {

		int bytes_to_read = 512;

		// check if we must extend the bfufer
		while (buffer_bytes + bytes_to_read > buffer_size) {
			extend_buffer();
		}

		// read some bytes
		int len = fread(&buffer[buffer_bytes], 1, bytes_to_read, file);
		if (len <= 0) {
			break;
		}
		buffer_bytes += len;
	}
	buffer[buffer_bytes] = 0;
}

/**
 *
 */
uint8_t* find_line(int line) {

	uint8_t* s = buffer;

	int current_line = 0;
	while (*s) {
		if (current_line == line) {
			break;
		}
		if (*s == '\n') {
			current_line++;
		}
		s++;
	}

	return s;
}

/**
 *
 */
int print_line(int line) {
	uint8_t* ls = find_line(line);

	int height = 1;
	if (ls) {
		int col = 0;
		while (*ls && *ls != '\n') {

			if (IS_PRINTED_CHAR(*ls)) {
				std::cout << *ls;
			}

			if (col > terminal_size.w) {
				std::cout << std::endl;
				height++;
				col = 0;
			}
			col++;
			ls++;
		}
	}
	return height;
}

/**
 *
 */
void display_current_lines() {

	g_terminal::clear();

	// put data on screen
	int line = scroll_line;
	while (line < terminal_size.h) {
		g_terminal::setCursor(g_term_cursor_position(0, line));
		line += print_line(line);
	}

	std::flush(std::cout);
}

