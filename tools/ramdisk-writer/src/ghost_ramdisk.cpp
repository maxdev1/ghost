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
#include <stdexcept>
#include <cstdint>
#include <cerrno>
#include <memory>
#include <vector>

static const uint32_t RAMDISK_PATH_MAX = 4096;
static const uint32_t RAMDISK_FILENAME_MAX = 512;

/**
 *
 */
int main(int argc, char** argv)
{
	ghost_ramdisk ramdisk;
	if(argc == 2)
	{
		if(strcmp(argv[1], "--help") == 0)
		{
			std::cout << std::endl;
			std::cout << "NAME" << std::endl;
			std::cout << "  Ghost Kernel ramdisk generator, by Max Schluessel" << std::endl;
			std::cout << std::endl;
			std::cout << "DESCRIPTION" << std::endl;
			std::cout << "  This program generates a Ghost ramdisk from a given source folder." << std::endl;
			std::cout << "  To do so, use the following command syntax:" << std::endl;
			std::cout << std::endl;
			std::cout << "\tpath/to/source path/to/target" << std::endl;
			std::cout << std::endl;
			return 0;
		}

		std::cerr << "error: unrecognized command line option '" << argv[1] << std::endl << std::endl;
		return 1;
	}

	if(argc >= 3)
	{

		if(argc == 4)
		{
			char* flag = argv[3];
			if(strcmp(flag, "-v") == 0)
			{
				ramdisk.verbose = true;
			}
		}

		try
		{
			ramdisk.create(argv[1], argv[2]);
		}
		catch(const std::exception& error)
		{
			std::cerr << "error: " << error.what() << std::endl;
			return 1;
		}
		return 0;
	} else
	{
		std::cerr << "usage: " << argv[0] << " path/to/source path/to/target" << std::endl;
		return 1;
	}
}

/**
 *
 */
static std::string ramdiskTrim(const std::string& text)
{
	auto first = text.find_first_not_of(" \r\n\t");
	if(first == std::string::npos)
		return "";

	auto last = text.find_last_not_of(" \r\n\t");
	return text.substr(first, last - first + 1);
}

/**
 *
 */
void ghost_ramdisk::create(const char* sourcePath, const char* targetPath)
{

	// read .rdignore if it exists
	std::ifstream rdignore(std::string(sourcePath) + "/.rdignore");
	if(rdignore.is_open())
	{
		std::string line;
		while(std::getline(rdignore, line))
		{
			line = ramdiskTrim(line);
			if(!line.empty())
				ignores.push_back(line);
		}
	}

	out.exceptions(std::ios::badbit | std::ios::failbit);
	out.open(targetPath, std::ios::out | std::ios::binary);

	std::cout << "status: packing folder \"" << sourcePath << "\" to ramdisk file \"" << targetPath << "\":" << std::endl;
	int64_t pos = out.tellp();
	writeRecursive(sourcePath, sourcePath, "", 0, 0, false);
	int64_t written = out.tellp() - pos;
	out.close();
	std::cout << "status: ramdisk successfully created, wrote " << written << " bytes" << std::endl;
}

static bool ramdiskIsLinkName(const char* name)
{
	auto length = strlen(name);
	if(length < 5)
		return false;

	auto suffix = name + length - 5;
	return suffix[0] == '.' &&
	       (suffix[1] == 'l' || suffix[1] == 'L') &&
	       (suffix[2] == 'i' || suffix[2] == 'I') &&
	       (suffix[3] == 'n' || suffix[3] == 'N') &&
	       (suffix[4] == 'k' || suffix[4] == 'K');
}

static void ramdiskWriteInteger(std::ostream& output, uint32_t value)
{
	char bytes[4];
	for(unsigned i = 0; i < sizeof(bytes); ++i)
		bytes[i] = static_cast<char>(value >> (8 * i));
	output.write(bytes, sizeof(bytes));
}

/**
 *
 */
void ghost_ramdisk::writeRecursive(const char* basePath, const char* path, const char* name, uint64_t contentLength, uint32_t parentId, bool isFile)
{

	// check whether to skip the file
	std::string basePathStr(basePath);
	std::string pathStr(path);
	for(std::string ign : ignores)
	{

		// starts with star?
		if(ign.find("*") == 0)
		{
			std::string part = ign.substr(1);
			if(pathStr.find(part) == pathStr.length() - part.length())
			{
				std::cout << "  skipping: " << path << std::endl;
				return;
			}
		}

		// ends with star?
		if(ign.find("*") == ign.length() - 1)
		{
			std::string part = ign.substr(0, ign.length() - 1);
			std::string absolutePartPath = basePathStr + "/" + part;

			if(pathStr.find(absolutePartPath) == 0)
			{
				std::cout << "  skipping: " << path << std::endl;
				return;
			}
		}

		// full paths
		std::string absolutePath = basePathStr + "/" + ign;
		if(absolutePath == pathStr)
		{
			std::cout << "  skipping: " << path << std::endl;
			return;
		}
	}

	std::string linkBytes;
	struct stat metadata;
	if(lstat(path, &metadata) == 0 && S_ISLNK(metadata.st_mode))
	{
		if(!ramdiskIsLinkName(name))
			throw std::runtime_error("native symbolic links must have a .link suffix");
		char target[RAMDISK_PATH_MAX];
		auto count = readlink(path, target, sizeof(target));
		if(count <= 0 || count >= RAMDISK_PATH_MAX)
			throw std::runtime_error("invalid native link target");

		for(ssize_t i = 0; i < count; ++i)
		{
			auto value = (uint8_t) target[i];
			if(value < 32 || value == 127)
				throw std::runtime_error("invalid native link target");
		}

		linkBytes = "target=" + std::string(target, count) + "\n";
		contentLength = linkBytes.size();
		isFile = true;
	}

	if(contentLength > INT32_MAX || strlen(name) >= RAMDISK_FILENAME_MAX)
		throw std::runtime_error(std::string("unsupported ramdisk entry size or name: ") + path);

	uint32_t entryId = idCounter++;

	if(verbose)
	{
		std::stringstream msg;
		msg << " " << entryId << ": " << path << (isFile ? "" : "/");
		std::cout << msg.str() << std::endl;
	}

	if(entryId > 0)
	{
		out.put(isFile ? 1 : 0);
		ramdiskWriteInteger(out, entryId);
		ramdiskWriteInteger(out, parentId);
		ramdiskWriteInteger(out, strlen(name));
		out.write(name, strlen(name));
	}

	if(isFile)
	{
		ramdiskWriteInteger(out, contentLength);

		if(!linkBytes.empty())
		{
			out.write(linkBytes.data(), linkBytes.size());
			return;
		}

		constexpr size_t bufferSize = 0x10000;
		std::vector<char> buffer(bufferSize);
		std::ifstream fileInput(path, std::ios::in | std::ios::binary);
		if(!fileInput.is_open())
			throw std::runtime_error(std::string("could not open ramdisk input: ") + path);
		for(int64_t remaining = contentLength; remaining > 0;)
		{
			auto count = std::min(int64_t(bufferSize), remaining);
			fileInput.read(buffer.data(), count);
			if(fileInput.gcount() != count)
				throw std::runtime_error(std::string("ramdisk input changed or could not be read: ") + path);
			out.write(buffer.data(), count);
			remaining -= count;
		}
		if(fileInput.peek() != std::char_traits<char>::eof())
			throw std::runtime_error(std::string("ramdisk input grew while packing: ") + path);
		fileInput.close();
	} else
	{
		std::unique_ptr<DIR, decltype(&closedir)> directory(opendir(path), closedir);
		dirent* entry;

		if(directory)
		{
			for(;;)
			{
				errno = 0;
				entry = readdir(directory.get());
				if(!entry)
				{
					if(errno) throw std::runtime_error(std::string("could not list ramdisk input: ") + path);
					break;
				}
				std::string entryPath = std::string(path) + '/' + entry->d_name;

				struct stat s;
				int32_t statr = lstat(entryPath.c_str(), &s);
				if(statr == 0)
				{

					if(S_ISREG(s.st_mode) || S_ISLNK(s.st_mode))
					{
						writeRecursive(basePath, entryPath.c_str(), entry->d_name, s.st_size, entryId, true);

					} else if(S_ISDIR(s.st_mode))
					{
						if(!(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0))
						{
							writeRecursive(basePath, entryPath.c_str(), entry->d_name, 0, entryId, false);
						}
					}
				} else
				{
					throw std::runtime_error("could not inspect ramdisk input: " + entryPath);
				}
			}

			if(closedir(directory.release()))
				throw std::runtime_error(std::string("could not close ramdisk input directory: ") + path);
		} else
		{
			throw std::runtime_error(std::string("could not open ramdisk input directory: ") + path);
		}
	}

	out.flush();
}
