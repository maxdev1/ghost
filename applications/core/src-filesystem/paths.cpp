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

#include "filesystem.hpp"

const char* filesystemToolName(const g_filesystem_tool_options& options)
{
	return options.folder ? "folder" : "file";
}

bool filesystemToolError(const g_filesystem_tool_options& options, const char* action, const std::string& path)
{
	coreReportError(filesystemToolName(options), action, path.c_str());
	return false;
}

bool filesystemToolPathValid(const std::string& path)
{
	const size_t limit = G_PATH_MAX;
	if(path.empty() || path.size() >= limit)
	{
		errno = path.empty() ? EINVAL : ENAMETOOLONG;
		return false;
	}

	size_t component = 0;
	for(char character: path)
	{
		if(character == '/')
			component = 0;
		else if(++component >= G_FILENAME_MAX)
		{
			errno = ENAMETOOLONG;
			return false;
		}
	}

	return true;
}

bool filesystemToolInfoRead(const std::string& path, g_filesystem_tool_info& info)
{
	if(!filesystemToolPathValid(path))
		return false;
	if(lstat(path.c_str(), &info.metadata) != 0)
		return false;

	g_fs_stat_data data;
	if(g_fs_stat_l(path.c_str(), &data, false) != G_FS_STAT_SUCCESS)
	{
		errno = EIO;
		return false;
	}

	info.mount = data.type == G_FS_NODE_TYPE_ROOT || data.type == G_FS_NODE_TYPE_MOUNTPOINT;
	return true;
}

bool filesystemToolAbsolute(const std::string& path, std::string& absolute)
{
	if(!filesystemToolPathValid(path))
		return false;

	char resolved[G_PATH_MAX];
	auto status = g_real_path(path.c_str(), resolved);
	if(status != G_FS_REAL_PATH_SUCCESS)
	{
		errno = status == G_FS_REAL_PATH_LOOP ? ELOOP : status == G_FS_REAL_PATH_NAME_TOO_LONG ? ENAMETOOLONG :
			status == G_FS_REAL_PATH_NOT_DIRECTORY ? ENOTDIR : status == G_FS_REAL_PATH_INVALID_LINK ? EINVAL :
			status == G_FS_REAL_PATH_NOT_FOUND ? ENOENT : EIO;
		return false;
	}

	absolute = resolved;
	while(absolute.size() > 1 && absolute.back() == '/')
		absolute.pop_back();
	return true;
}

bool filesystemToolChildren(const g_filesystem_tool_options& options, const std::string& path,
							std::vector<std::string>& names)
{
	auto directory = opendir(path.c_str());
	if(!directory)
		return filesystemToolError(options, "list", path);

	bool success = true;
	for(;;)
	{
		errno = 0;
		auto entry = readdir(directory);
		if(!entry)
		{
			if(errno)
				success = filesystemToolError(options, "list", path);
			break;
		}

		if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
			names.emplace_back(entry->d_name);
	}

	if(closedir(directory) != 0)
		success = filesystemToolError(options, "close directory", path);
	std::sort(names.begin(), names.end());
	return success;
}

/**
 * Matches a whole entry name using '*' for any sequence and '?' for one byte.
 * Matching is case-sensitive and does not interpret path separators or brackets.
 */
static bool filesystemToolMatch(const char* pattern, const char* name)
{
	const char* star = nullptr;
	const char* retry = nullptr;
	while(*name)
	{
		if(*pattern == '?' || (*pattern && *pattern == *name))
		{
			++pattern;
			++name;
		}
		else if(*pattern == '*')
		{
			star = pattern++;
			retry = name;
		}
		else if(star)
		{
			pattern = star + 1;
			name = ++retry;
		}
		else
			return false;
	}

	while(*pattern == '*')
		++pattern;
	return !*pattern;
}

bool filesystemToolBrowse(const g_filesystem_tool_options& options, const std::string& path,
							const std::string& relative, unsigned depth, dev_t device, uint64_t& total)
{
	if(depth >= options.depth)
		return true;

	std::vector<std::string> names;
	if(!filesystemToolChildren(options, path, names))
		return false;

	for(const auto& name: names)
	{
		auto child = path + "/" + name;
		auto display = relative.empty() ? name : relative + "/" + name;
		g_filesystem_tool_info info;
		if(!filesystemToolInfoRead(child, info))
			return filesystemToolError(options, "inspect", child);

		bool directory = S_ISDIR(info.metadata.st_mode);
		bool regular = S_ISREG(info.metadata.st_mode);
		bool boundary = info.mount || info.metadata.st_dev != device;
		if(options.action == "size")
		{
			if(regular)
			{
				if(info.metadata.st_size < 0 || uint64_t(info.metadata.st_size) > UINT64_MAX - total)
				{
					errno = EOVERFLOW;
					return filesystemToolError(options, "measure", child);
				}

				total += info.metadata.st_size;
			}
		}
		else if(options.action == "tree")
		{
			for(unsigned i = 0; i < depth; ++i)
				printf("  ");
			printf("%s%s%s\n", name.c_str(), directory ? "/" : "", boundary ? " [mount]" : "");
		}
		else if(options.action == "find")
		{
			if(filesystemToolMatch(options.argument.c_str(), name.c_str()) &&
			   (options.type.empty() || (options.type == "folder" && directory) ||
				(options.type == "file" && regular) || (options.type == "link" && S_ISLNK(info.metadata.st_mode))))
				printf("%s\n", display.c_str());
		}
		else if(options.detailed)
		{
			const char* type = boundary ? "mount" : S_ISLNK(info.metadata.st_mode) ? "link" : directory ? "folder" : regular ? "file" : "other";
			if(regular && info.metadata.st_size >= 0)
				printf("%s\t%llu\t%s\n", type, (unsigned long long) info.metadata.st_size, display.c_str());
			else
				printf("%s\t-\t%s\n", type, display.c_str());
		}
		else
			printf("%s%s\n", display.c_str(), directory ? "/" : "");

		if(directory && !boundary && (options.recursive || options.action != "list"))
			if(!filesystemToolBrowse(options, child, display, depth + 1, device, total))
				return false;
	}

	return true;
}
