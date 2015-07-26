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

#ifndef __LAUNCHSCRIPT__
#define __LAUNCHSCRIPT__

#include <stdio.h>
#include <string>
#include <vector>

/**
 *
 */
class ls_pair_t {
public:
	std::string key;
	std::string value;
};

/**
 *
 */
class ls_statement_t {
public:
	std::vector<ls_pair_t*> pairs;
};

/**
 *
 */
class ls_document_t {
public:
	std::vector<ls_statement_t*> statements;
};

/**
 *
 */
class launchscript_parser_t {
private:
	FILE* file;
	char c;

public:
	launchscript_parser_t(FILE* file);

	void step();

	ls_document_t* document();
	ls_statement_t* statement();
	ls_pair_t* pair();
	std::string key();
	std::string value();
};

#endif
