/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *                 Changes, a file change monitoring util                    *
 *                   Copyright (C) 2014, Max Schluessel                      *
 *                                                                           *
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

#ifndef __CHANGES__
#define __CHANGES__

#include "system.hpp"

#include <map>
#include <string>

#define DEFAULT_TABLEPATH		".changes"

#define FLAG_CHECK				"-c"
#define FLAG_CHECK_L			"--check"
#define FLAG_STORE				"-s"
#define FLAG_STORE_L			"--store"
#define FLAG_OUT				"-o"
#define FLAG_OUT_L				"--output"
#define FLAG_HELP				"--help"
#define FLAG_VERSION			"--version"
#define FLAG_CLEAR				"--clear"

enum changes_mode_t {
	CHANGES_MODE_NONE, CHANGES_MODE_STORE, CHANGES_MODE_CHECK, CHANGES_MODE_CLEAR
};

void title();
int help(char* self);
int version();
int unknown_flag(char* self, std::string flag);
int usage_message(char* self);
std::map<std::string, std::string> parse_commands(int argc, char** argv);

void store(std::string path, std::string tablepath);
int check(std::string path, std::string tablepath);
void clear(std::string tablepath);

void read_change_table(std::string path, std::map<std::string, long>& target);
void save_change_table(std::string path, std::map<std::string, long>& target);

std::string get_absolute_path(std::string path);
long get_last_modify_date(std::string path);

#endif
