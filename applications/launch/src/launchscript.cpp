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

#include "launchscript.hpp"
#include <sstream>

/**
 *
 */
launchscript_parser_t::launchscript_parser_t(FILE* file) :
		file(file) {
	step();
}

/**
 *
 */
void launchscript_parser_t::step() {
	c = fgetc(file);
}

/**
 *
 */
ls_document_t* launchscript_parser_t::document() {

	ls_document_t* doc = new ls_document_t;

	while (true) {
		ls_statement_t* stat = statement();
		if (stat == 0) {
			break;
		}
		doc->statements.push_back(stat);
	}

	return doc;
}

/**
 *
 */
ls_statement_t* launchscript_parser_t::statement() {

	ls_statement_t* stat = new ls_statement_t;

	// skip line end & spaces
	while (c == '\n' || isspace(c) || c == '#') {
		if (c == '#') {
			while (c != '\n') {
				step();
			}
			step();
		} else {
			step();
		}
	}

	// read pairs
	while (true) {
		ls_pair_t* p = pair();
		if (p == 0) {
			break;
		}
		stat->pairs.push_back(p);
	}

	// no pairs? script finished
	if (stat->pairs.size() == 0) {
		delete stat;
		return 0;
	}

	return stat;
}

/**
 *
 */
ls_pair_t* launchscript_parser_t::pair() {

	ls_pair_t* p = new ls_pair_t;

	// key
	std::string k = key();
	if (k.empty()) {
		delete p;
		return 0;
	}

	// ':'
	if (c == ':') {
		step();
	}

	// value
	std::string v = value();
	if (v.empty()) {
		delete p;
		return 0;
	}

	p->key = k;
	p->value = v;

	return p;
}

/**
 *
 */
std::string launchscript_parser_t::key() {

	while (c != '\n' && isspace(c)) {
		step();
	}

	std::stringstream ks;
	while (c != ':' && c != '\n' && c != EOF) {
		ks << c;
		step();
	}
	return ks.str();
}

/**
 *
 */
std::string launchscript_parser_t::value() {

	while (c != '\n' && isspace(c)) {
		step();
	}

	bool instr = false;
	bool esc = false;

	std::stringstream ks;
	while (c != EOF) {
		if (c == '"' && !esc) {
			instr = !instr;
			esc = false;
		} else if (c == '\\' && !esc) {
			esc = true;
		} else if (isspace(c) && !esc) {
			break;
		} else {
			ks << c;
			esc = false;
		}
		step();
	}
	return ks.str();
}
