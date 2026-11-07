/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2026, Max Schlüssel <lokoxe@gmail.com>                     *
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

#ifndef CORE_FILESYSTEM_TOOL_HPP
#define CORE_FILESYSTEM_TOOL_HPP

#include "../src-common/arguments.hpp"
#include "../src-common/io.hpp"
#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <vector>

#include <ghost/filesystem.h>

struct g_filesystem_tool_info
{
	struct stat metadata{};
	bool mount = false;
};

struct g_filesystem_tool_options
{
	std::string path;
	std::string action;
	std::string argument;
	bool folder = false;
	bool replace = false;
	bool parents = false;
	bool recursive = false;
	bool detailed = false;
	bool storage = false;
	unsigned depth = UINT_MAX;
	std::string type;
};

const char* filesystemToolName(const g_filesystem_tool_options& options);
bool filesystemToolError(const g_filesystem_tool_options& options, const char* action, const std::string& path);
bool filesystemToolPathValid(const std::string& path);
bool filesystemToolInfoRead(const std::string& path, g_filesystem_tool_info& info);
bool filesystemToolAbsolute(const std::string& path, std::string& absolute);
bool filesystemToolChildren(const g_filesystem_tool_options& options, const std::string& path,
	std::vector<std::string>& names);
bool filesystemToolCopy(const g_filesystem_tool_options& options, const std::string& source,
	const std::string& destination, bool root = true);
bool filesystemToolDelete(const g_filesystem_tool_options& options, const std::string& path);
bool filesystemToolBrowse(const g_filesystem_tool_options& options, const std::string& path,
	const std::string& relative, unsigned depth, dev_t device, uint64_t& total);
int filesystemToolRun(int argc, char** argv, bool folder);

#endif
