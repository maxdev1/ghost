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

#include "../inc/ghost_ramdisk.hpp"

#include <iostream>
#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <algorithm>
#include <list>

/**
 *
 */
int main(int argc, char** argv) {

	ghost_ramdisk ramdisk;
	if (argc == 2) {
		if (strcmp(argv[1], "--help") == 0) {
			std::cout << std::endl;
			std::cout << "NAME" << std::endl;
			std::cout << "  Ghost Kernel ramdisk generator, by Max Schluessel"
					<< std::endl;
			std::cout << std::endl;
			std::cout << "DESCRIPTION" << std::endl;
			std::cout
					<< "  This program generates a Ghost ramdisk from a given source folder."
					<< std::endl;
			std::cout << "  To do so, use the following command syntax:"
					<< std::endl;
			std::cout << std::endl;
			std::cout << "\tpath/to/source path/to/target" << std::endl;
			std::cout << std::endl;
			return 0;
		}

		std::cerr << "error: unrecognized command line option '" << argv[1]
				<< std::endl << std::endl;
		return 1;
	}

	if (argc == 3) {
		ramdisk.create(argv[1], argv[2]);
		return 0;
	} else {
		std::cerr << "usage: " << argv[0] << " path/to/source path/to/target"
				<< std::endl;
		return 1;
	}
}

/**
 *
 */
std::string trim(std::string& str) {
	if (str.length() > 0) {
		size_t first = str.find_first_not_of(" \r\n\t");
		size_t last = str.find_last_not_of(" \r\n\t");
		return str.substr(first, (last - first + 1));
	}
	return str;
}

/**
 *
 */
void ghost_ramdisk::create(const char* sourcePath, const char* targetPath) {

	// read .rdignore if it exists
	std::ifstream rdignore(std::string(sourcePath) + "/.rdignore");
	if (rdignore.is_open()) {
		std::string line;
		while (std::getline(rdignore, line)) {
			if (line.length() > 0) {
				ignores.push_back(trim(line));
			}
		}
	}

	try {
		out.open(targetPath, std::ios::out | std::ios::binary);

		if (out.good()) {
			std::cout << "status: packing folder \"" << sourcePath
					<< "\" to ramdisk file \"" << targetPath << "\":"
					<< std::endl;
			int64_t pos = out.tellp();
			writeRecursive(sourcePath, sourcePath, "", 0, 0, false);
			int64_t written = out.tellp() - pos;
			std::cout << "status: ramdisk successfully created, wrote "
					<< written << " bytes" << std::endl;
		} else {
			std::cerr << "error: could not write to file '" << targetPath << "'"
					<< std::endl;
		}

	} catch (std::exception& e) {
		std::cerr << "error: an error occured while creating the ramdisk"
				<< std::endl;
	}

	if (out.is_open()) {
		out.close();
	}
}

/**
 *
 */
void ghost_ramdisk::writeRecursive(const char* basePath, const char* path,
		const char* name, uint32_t contentLength, uint32_t parentId,
		bool isFile) {

	// check whether to skip the file
	std::string basePathStr(basePath);
	std::string pathStr(path);
	for (std::string ign : ignores) {

		// starts with star?
		if (ign.find("*") == 0) {
			std::string part = ign.substr(1);
			if (pathStr.find(part) == pathStr.length() - part.length()) {
				std::cout << "  skipping: " << path << std::endl;
				return;
			}
		}

		// ends with star?
		if (ign.find("*") == ign.length() - 1) {
			std::string part = ign.substr(0, ign.length() - 1);
			std::string absolutePartPath = basePathStr + "/" + part;

			if (pathStr.find(absolutePartPath) == 0) {
				std::cout << "  skipping: " << path << std::endl;
				return;
			}
		}

		// full paths
		std::string absolutePath = basePathStr + "/" + ign;
		if (absolutePath == pathStr) {
			std::cout << "  skipping: " << path << std::endl;
			return;
		}
	}

	uint32_t bufferSize = 0x10000;
	char* buffer = new char[bufferSize];
	uint32_t entryId = idCounter++;

	std::stringstream msg;
	msg << " " << entryId << ": " << path << (isFile ? "" : "/");
	std::cout << msg.str() << std::endl;

	// Root must not be written
	if (entryId > 0) {
		// file or folder
		buffer[0] = isFile ? 1 : 0;
		out.write(buffer, 1);

		// id
		buffer[0] = ((entryId >> 0) & 0xFF);
		buffer[1] = ((entryId >> 8) & 0xFF);
		buffer[2] = ((entryId >> 16) & 0xFF);
		buffer[3] = ((entryId >> 24) & 0xFF);
		out.write(buffer, 4);

		// parent id
		buffer[0] = ((parentId >> 0) & 0xFF);
		buffer[1] = ((parentId >> 8) & 0xFF);
		buffer[2] = ((parentId >> 16) & 0xFF);
		buffer[3] = ((parentId >> 24) & 0xFF);
		out.write(buffer, 4);

		// name length
		int32_t namelen = strlen(name);
		buffer[0] = ((namelen >> 0) & 0xFF);
		buffer[1] = ((namelen >> 8) & 0xFF);
		buffer[2] = ((namelen >> 16) & 0xFF);
		buffer[3] = ((namelen >> 24) & 0xFF);
		out.write(buffer, 4);

		// name
		memcpy(buffer, name, namelen);
		out.write(buffer, namelen);
	}

	if (isFile) {
		// file length
		buffer[0] = ((contentLength >> 0) & 0xFF);
		buffer[1] = ((contentLength >> 8) & 0xFF);
		buffer[2] = ((contentLength >> 16) & 0xFF);
		buffer[3] = ((contentLength >> 24) & 0xFF);
		out.write(buffer, 4);

		// file content
		std::ifstream fileInput;
		fileInput.open(path, std::ios::in | std::ios::binary);

		int32_t length;
		while (fileInput.good()) {
			fileInput.read(buffer, bufferSize);
			length = fileInput.gcount();
			out.write(buffer, length);
		}

		fileInput.close();
	} else {
		DIR *directory;
		dirent *entry;

		if ((directory = opendir(path)) != NULL) {
			while ((entry = readdir(directory)) != NULL) {
				char entryPath[260];

				std::stringstream str;
				str << path;
				str << '/';
				str << entry->d_name;

				str >> entryPath;

				struct stat s;
				int32_t statr = stat(entryPath, &s);
				if (statr == 0) {

					if (s.st_mode & S_IFREG) {
						writeRecursive(basePath, entryPath, entry->d_name,
								s.st_size, entryId, true);

					} else if (s.st_mode & S_IFDIR) {
						if (!(strcmp(entry->d_name, ".") == 0
								|| strcmp(entry->d_name, "..") == 0)) {
							writeRecursive(basePath, entryPath, entry->d_name,
									0, entryId, false);
						}
					}
				} else {
					std::cerr << "error: could not read directory: '" << path
							<< "'";
					break;
				}
			}

			closedir(directory);
		} else {
			std::cerr << "error: could not open directory: '" << path << "'";
		}
	}

	// entry done
	out.flush();

	delete buffer;
}
