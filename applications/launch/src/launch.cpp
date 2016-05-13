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

#include "launch.hpp"
#include <stdio.h>
#include <ghost.h>
#include <ghostuser/utils/logger.hpp>

/**
 *
 */
int main(int argc, char** argv) {

	// check parameters
	char* path;
	if (argc > 1) {
		path = argv[1];
	} else {
		fprintf(stderr, "usage:		%s /path/to/script\n", argv[0]);
		return -1;
	}

	// load given script
	FILE* scriptfile = fopen(path, "r");
	if (scriptfile == NULL) {
		fprintf(stderr, "%s: file not found\n", path);
		return -1;
	}

	// parse the script
	launchscript_parser_t* parser = new launchscript_parser_t(scriptfile);
	ls_document_t* document = parser->document();

	// interpret script
	for (ls_statement_t* stat : document->statements) {

		std::string command = stat->pairs[0]->key;

		if (command == "print") {
			print(stat);

		} else if (command == "driver") {
			driver(stat);

		} else if (command == "application") {
			application(stat);

		} else if (command == "wait") {
			wait(stat);
		}
	}

	return 0;
}

/**
 *
 */
void print(ls_statement_t* stat) {
	klog(stat->pairs[0]->value.c_str());
}

/**
 *
 */
std::string find_param(ls_statement_t* stat, std::string key, std::string def) {

	for (auto p : stat->pairs) {
		if (p->key == key) {
			return p->value;
		}
	}
	return def;
}

/**
 *
 */
void driver(ls_statement_t* stat) {

	std::string path = stat->pairs[0]->value;
	std::string args = find_param(stat, "args", "");
	launch_via_spawner(path, args, G_SECURITY_LEVEL_DRIVER);
}

/**
 *
 */
void application(ls_statement_t* stat) {

	std::string path = stat->pairs[0]->value;
	std::string args = find_param(stat, "args", "");
	launch_via_spawner(path, args, G_SECURITY_LEVEL_APPLICATION);
}

/**
 *
 */
void wait(ls_statement_t* stat) {

	std::string identifier = stat->pairs[0]->value;
	klog(("waiting for '" + identifier + "'").c_str());
	while (g_task_get_id(identifier.c_str()) == -1) {
		g_yield();
	}
}

/**
 *
 */
g_pid launch_via_spawner(std::string path, std::string args, g_security_level sec_lvl) {

	g_pid pid = -1;
	g_spawn_status stat = g_spawn_p(path.c_str(), args.c_str(), "/", sec_lvl, &pid);
	if (stat != G_SPAWN_STATUS_SUCCESSFUL) {
		g_logger::log("failed to spawn '" + path + "' with code %i", stat);
	}
	return pid;
}
