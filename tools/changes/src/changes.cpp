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

#include "changes.hpp"

using namespace std;

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>

/**
 *
 */
int main(int argc, char** argv) {

	map < string, string > params = parse_commands(argc, argv);

	char* self = argv[0];
	changes_mode_t mode = CHANGES_MODE_NONE;
	string file;
	string tablepath = DEFAULT_TABLEPATH;

	// read parameters
	for (auto entry : params) {

		if (entry.first == FLAG_CHECK || entry.first == FLAG_CHECK_L) {
			if (mode != CHANGES_MODE_NONE) {
				return usage_message(self);
			}
			mode = CHANGES_MODE_CHECK;
			file = entry.second;

		} else if (entry.first == FLAG_STORE || entry.first == FLAG_STORE_L) {
			if (mode != CHANGES_MODE_NONE) {
				return usage_message(self);
			}
			mode = CHANGES_MODE_STORE;
			file = entry.second;

		} else if (entry.first == FLAG_CLEAR) {
			if (mode != CHANGES_MODE_NONE) {
				return usage_message(self);
			}
			mode = CHANGES_MODE_CLEAR;

		} else if (entry.first == FLAG_OUT || entry.first == FLAG_OUT_L) {
			tablepath = entry.second;

		} else if (entry.first == FLAG_HELP) {
			return help(self);

		} else if (entry.first == FLAG_VERSION) {
			return version();

		} else {
			return unknown_flag(self, entry.first);
		}
	}

	if (mode == CHANGES_MODE_NONE) {
		return usage_message(self);
	}

	// check if file exists
	string absolute = get_absolute_path(file);
	if (mode == CHANGES_MODE_CHECK || mode == CHANGES_MODE_STORE) {
		if (access(absolute.c_str(), F_OK) == -1) {
			fprintf(stderr, "error: '%s' does not exist\n", absolute.c_str());
			return -1;
		}
	}

	if (mode == CHANGES_MODE_CHECK) {
		return check(absolute, tablepath);
	} else if (mode == CHANGES_MODE_STORE) {
		store(absolute, tablepath);
		return 0;
	} else if (mode == CHANGES_MODE_CLEAR) {
		clear(tablepath);
		return 0;
	}
}
/**
 *
 */
map<string, string> parse_commands(int argc, char** argv) {
	map < string, string > values;

	string flag;
	bool atflag = true;

	for (int i = 1; i < argc; i++) {
		string param = argv[i];

		if (param.size() == 0) {
			continue;
		}

		if (atflag) {
			flag = param;

			if (flag == FLAG_HELP || flag == FLAG_VERSION || flag == FLAG_CLEAR) {
				values[flag] = "";
			} else {
				atflag = false;
			}
		} else {
			values[flag] = param;
			atflag = true;
		}
	}

	if (!atflag) {
		values[flag] = "";
	}

	return values;
}

/**
 *
 */
void store(string path, string tablepath) {
	long lastModify = get_last_modify_date(path);
	map<string, long> entries;
	read_change_table(tablepath, entries);

	entries[path] = lastModify;
	save_change_table(tablepath, entries);
}

/**
 *
 */
int check(string path, string tablepath) {
	long lastModify = get_last_modify_date(path);
	map<string, long> entries;
	read_change_table(tablepath, entries);

	for (auto entry : entries) {
		if (path == entry.first) {
			return (lastModify > entry.second) ? 1 : 0;
		}
	}
	return 1;
}

/**
 *
 */
void clear(string tablepath) {
	map<string, long> entries;
	read_change_table(tablepath, entries);
	entries.clear();
	save_change_table(tablepath, entries);
}

/**
 *
 */
void title() {
	printf("CHANGES, a file change monitoring tool\n"
			"\n");
}

/**
 *
 */
int help(char* self) {
	title();

	printf("This program can be used to check if a file has changed. The following commands are available:\n");
	printf("\n");
	printf("   %s, %s\n", FLAG_STORE, FLAG_STORE_L);

	// -----################################################################################----- 80 chars
	printf("       saves the current modify date of the given file\n");
	printf("   %s, %s\n", FLAG_CHECK, FLAG_CHECK_L);
	printf("       returns 1 if the specified file has changed since the last use of store,\n");
	printf("       returns 0 if it did not change\n");
	printf("   %s\n", FLAG_CLEAR);
	printf("       removes all entries from the change table\n");
	printf("   %s, %s\n", FLAG_OUT, FLAG_OUT_L);
	printf("       sets the location of the used change table\n");
	printf("   %s\n", FLAG_HELP);
	printf("       displays this help message\n");
	printf("   %s\n", FLAG_VERSION);
	printf("       displays the current version\n");
	printf("\n");
	printf("Modify dates are saved in a file named '%s' by default. To change the path for this file, use '%s alternatename'.\n", DEFAULT_TABLEPATH, FLAG_OUT);
	printf("\n");
	printf("Examples:\n");
	printf("   %s %s test.txt\n", self, FLAG_STORE);
	printf("       stores the current modify date for the file\n");
	printf("   %s %s test.txt\n", self, FLAG_CHECK);
	printf("       returns whether the file was modified since the last store\n");
	printf("   %s %s test.txt %s .mytable\n", self, FLAG_STORE, FLAG_OUT);
	printf("       stores the current modify date in the specified table file\n");
	printf("\n");
	printf("  - Max Schluessel");
	printf("\n");

	return 0;
}

/**
 *
 */
int version() {
	title();
	printf("version 0.1\n");
	return 0;
}

/**
 *
 */
int usage_message(char* self) {
	std::stringstream f;
	f << "[";
	f << FLAG_CHECK << ",";
	f << FLAG_STORE;
	f << "]";
	fprintf(stderr, "usage:\n\t%s %s file\n", self, f.str().c_str());
	fprintf(stderr, "consider '%s' for more information", FLAG_HELP);
	return -1;
}

/**
 *
 */
int unknown_flag(char* self, string flag) {
	fprintf(stderr, "error: unknown flag '%s'\n", flag.c_str());
	return usage_message(self);
}

/**
 *
 */
string get_absolute_path(string path) {
	char* buf = new char[PATH_MAX];

#if WINDOWS
	GetFullPathName(path.c_str(), PATH_MAX, buf, 0);
#elif MAC_OS or LINUX
	realpath(path.c_str(), buf);
#endif

	string absolute = buf;
	delete buf;
	return absolute;
}

/**
 *
 */
long get_last_modify_date(string path) {
	struct stat attrib;
	stat(path.c_str(), &attrib);
	tm modifyTime = *gmtime(&(attrib.st_mtime));
	return mktime(&modifyTime);
}

/**
 *
 */
void read_change_table(string path, map<string, long>& target) {

	ifstream in;
	in.open(path);

	if (!in.is_open()) {
		return;
	}

	string line;
	while (getline(in, line)) {
		int eqpos = line.find_last_of("=");
		if (eqpos != -1) {
			string filepath = line.substr(0, eqpos);
			string timestamps = line.substr(eqpos + 1);

			stringstream s;
			s << timestamps;
			long timestamp;
			s >> timestamp;

			if (!filepath.empty() && !timestamps.empty()) {
				target[filepath] = timestamp;
			}
		}
	}

	in.close();
}

/**
 *
 */
void save_change_table(string path, map<string, long>& target) {

	ofstream out;
	out.open(path);

	if (!out.is_open()) {
		return;
	}

	for (auto entry : target) {
		out << entry.first << "=" << entry.second << "\n";
	}

	out.close();
}
