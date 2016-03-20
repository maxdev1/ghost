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

#ifndef __GHOST_RAMDISK__
#define __GHOST_RAMDISK__

#include <fstream>
#include <stdint.h>
#include <list>

#define VERSION_MAJOR	1
#define	VERSION_MINOR	0

/**
 *
 */
class ghost_ramdisk {
private:
	int idCounter;
	std::ofstream out;
	std::list<std::string> ignores;

	void writeRecursive(const char* basePath, const char* path, const char* name, uint32_t contentLength, uint32_t parentId, bool isFile);

public:
	ghost_ramdisk() :
			idCounter(0) {
	}

	void create(const char* sourcePath, const char* targetPath);
};

#endif
