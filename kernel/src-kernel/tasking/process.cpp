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

#include <tasking/process.hpp>

/**
 *
 */
g_process::g_process(g_security_level securityLevel) :
		securityLevel(securityLevel) {
	parent = 0;
	main = 0;

	pageDirectory = 0;

	imageStart = 0;
	imageEnd = 0;
	heapStart = 0;
	heapBreak = 0;
	heapPages = 0;

	// empty string fields
	cliArguments = 0;
	source_path = 0;

	// set default working directory
	workingDirectory = new char[G_PATH_MAX];
	workingDirectory[0] = '/';
	workingDirectory[1] = 0;

	tls_master_in_proc_location = 0;
	tls_master_copysize = 0;
	tls_master_totalsize = 0;
	tls_master_alignment = 0;
}

/**
 *
 */
g_process::~g_process() {

	if (cliArguments) {
		delete cliArguments;
	}

	if (source_path) {
		delete source_path;
	}

}

