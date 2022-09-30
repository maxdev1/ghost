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

#ifndef __GHOST_GOSH_PARSER__
#define __GHOST_GOSH_PARSER__

#include <vector>
#include <string>

struct pipe_expression_t;
struct program_call_t;

/**
 *
 */
struct pipe_expression_t {
	std::vector<program_call_t*> calls;
};

/**
 *
 */
struct program_call_t {
	std::string program;
	std::vector<std::string> arguments;
};

/**
 *
 */
class parser_t {
private:
	char current;
	int position;
	std::string input;

	void step();
	void skip_whitespace();
public:
	parser_t(std::string input);

	bool pipe_expression(pipe_expression_t** out);
	bool program_call(program_call_t** out);
	bool argument(std::string& out);
};

#endif
