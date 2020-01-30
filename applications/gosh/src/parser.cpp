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

#include "parser.hpp"
#include <iostream>
#include <ghostuser/utils/local.hpp>

/**
 *
 */
parser_t::parser_t(std::string input) :
		input(input) {
	position = 0;
	current = -1;
	step();
}

/**
 *
 */
void parser_t::step() {
	if (position < input.size()) {
		current = input[position++];
	} else {
		current = -1;
	}
}

/**
 *
 */
void parser_t::skip_whitespace() {
	while (isspace(current)) {
		step();
	}
}

/**
 *
 */
bool parser_t::pipe_expression(pipe_expression_t** out) {

	g_local<pipe_expression_t> expr(new pipe_expression_t());

	while (true) {
		skip_whitespace();

		// program call
		program_call_t* pc;
		if (program_call(&pc)) {
			expr()->calls.push_back(pc);
			skip_whitespace();

			// optionally a pipe
			if (current == '|') {
				step();

				// end of input
			} else if (current == -1) {
				break;

			} else {
				std::cout << (char) 27 << "[31m";
				std::cout << "unexpected character: " << current << std::endl;
				std::cout << (char) 27 << "[0m";
				return false;
			}

		} else {
			std::cout << (char) 27 << "[31m";
			std::cerr << "syntax error: expected a program call at position: "
					<< position << std::endl;
			std::cout << (char) 27 << "[0m";
			return false;
		}
	}

	*out = expr.release();
	return true;
}

/**
 *
 */
bool parser_t::program_call(program_call_t** out) {

	g_local<program_call_t> expr(new program_call_t());

	// read until space or pipe
	std::string program = "";
	while (current != -1 && current != '|' && !isspace(current)) {
		program += current;
		step();
	}
	if (program.length() == 0) {
		return false;
	}
	expr()->program = program;

	// parse arguments
	std::string arg;
	while (argument(arg)) {
		expr()->arguments.push_back(arg);
	}

	*out = expr.release();
	return true;
}

/**
 *
 */
bool parser_t::argument(std::string& out) {

	// skip whitespace
	skip_whitespace();

	// fin on end
	if (current == -1) {
		return false;
	}

	// start on pipe
	if (current == '|') {
		return false;
	}

	// parse content
	std::string arg = "";

	bool instr = false;
	bool escaped = false;

	while (true) {
		// stop on end
		if (current == -1) {
			break;
		}

		if (!escaped && current == '\\') {
			escaped = true;

		} else if (instr) {
			if (current == '"' && !escaped) {
				instr = false;
			} else {
				arg += current;
			}

			if (escaped) {
				escaped = false;
			}

		} else {
			if (!escaped && (current == '|' || current == ' ')) {
				break;
			} else if (current == '"' && !escaped) {
				instr = true;
			} else {
				arg += current;
			}

			if (escaped) {
				escaped = false;
			}
		}

		step();
	}

	out = arg;
	return true;
}
