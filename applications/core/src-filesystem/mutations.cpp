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

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

bool filesystemToolCopy(const g_filesystem_tool_options& options, const std::string& source,
						 const std::string& destination, bool root)
{
	if(!filesystemToolPathValid(destination))
		return filesystemToolError(options, "use destination", destination);
	g_filesystem_tool_info input;
	if(!filesystemToolInfoRead(source, input))
		return filesystemToolError(options, "inspect", source);

	if(S_ISDIR(input.metadata.st_mode))
	{
		if(!root && input.mount)
		{
			errno = EBUSY;
			return filesystemToolError(options, "copy mounted directory", source);
		}

		std::vector<std::string> names;
		if(!filesystemToolChildren(options, source, names))
			return false;
		if(mkdir(destination.c_str(), 0777) != 0)
			return filesystemToolError(options, "create", destination);

		for(const auto& name: names)
		{
			auto child = source + "/" + name;
			g_filesystem_tool_info info;
			if(!filesystemToolInfoRead(child, info))
				return filesystemToolError(options, "inspect", child);
			if(info.mount || info.metadata.st_dev != input.metadata.st_dev)
			{
				errno = EBUSY;
				return filesystemToolError(options, "cross mount", child);
			}

			if(!filesystemToolCopy(options, child, destination + "/" + name, false))
				return false;
		}

		return true;
	}

	if(!S_ISREG(input.metadata.st_mode) && !S_ISLNK(input.metadata.st_mode))
	{
		errno = EINVAL;
		return filesystemToolError(options, "copy unsupported node", source);
	}

	int descriptor = open(source.c_str(), O_RDONLY | O_LINKFILE | O_NOFOLLOW);
	if(descriptor < 0)
		return filesystemToolError(options, "open", source);

	struct stat opened;
	if(fstat(descriptor, &opened) != 0 || (!S_ISREG(opened.st_mode) && !S_ISLNK(opened.st_mode)))
	{
		errno = EINVAL;
		filesystemToolError(options, "copy", source);
		coreClose(descriptor, filesystemToolName(options), source.c_str());
		return false;
	}

	bool success = true;
	g_filesystem_tool_info existing;
	bool exists = filesystemToolInfoRead(destination, existing);
	if((exists && (!root || !options.replace || (!S_ISREG(existing.metadata.st_mode) && !S_ISLNK(existing.metadata.st_mode)) ||
		(existing.metadata.st_dev == opened.st_dev && existing.metadata.st_ino == opened.st_ino))) ||
		(!exists && errno != ENOENT))
	{
		if(exists)
			errno = EEXIST;
		success = filesystemToolError(options, "copy to", destination);
	}

	int output = -1;
	if(success)
	{
		output = open(destination.c_str(), O_WRONLY | O_CREAT | O_LINKFILE | O_NOFOLLOW |
					  (exists ? 0 : O_EXCL), 0666);
		if(output < 0)
			success = filesystemToolError(options, "create", destination);
	}

	if(output >= 0)
	{
		struct stat target;
		if(fstat(output, &target) != 0 || (!S_ISREG(target.st_mode) && !S_ISLNK(target.st_mode)) ||
		   (target.st_dev == opened.st_dev && target.st_ino == opened.st_ino))
		{
			errno = EINVAL;
			success = filesystemToolError(options, "copy to", destination);
		}
		else if(exists && ftruncate(output, 0) != 0)
			success = filesystemToolError(options, "resize", destination);
		else
			success = coreTransfer(descriptor, output, filesystemToolName(options), source.c_str(),
							   destination.c_str()) == 0;

		if(coreClose(output, filesystemToolName(options), destination.c_str()))
			success = false;
	}

	if(coreClose(descriptor, filesystemToolName(options), source.c_str()))
		success = false;
	return success;
}

bool filesystemToolDelete(const g_filesystem_tool_options& options, const std::string& path)
{
	g_filesystem_tool_info info;
	if(!filesystemToolInfoRead(path, info))
		return filesystemToolError(options, "inspect", path);

	if(info.mount)
	{
		errno = EBUSY;
		return filesystemToolError(options, "delete mounted directory", path);
	}

	if(S_ISDIR(info.metadata.st_mode))
	{
		std::vector<std::string> names;
		if(!filesystemToolChildren(options, path, names))
			return false;

		for(const auto& name: names)
		{
			auto child = path + "/" + name;
			g_filesystem_tool_info childInfo;
			if(!filesystemToolInfoRead(child, childInfo))
				return filesystemToolError(options, "inspect", child);
			if(childInfo.mount || childInfo.metadata.st_dev != info.metadata.st_dev)
			{
				errno = EBUSY;
				return filesystemToolError(options, "delete mounted directory", child);
			}

			if(!filesystemToolDelete(options, child))
				return false;
		}

		if(rmdir(path.c_str()) != 0)
			return filesystemToolError(options, "delete", path);
	}
	else if(S_ISREG(info.metadata.st_mode) || S_ISLNK(info.metadata.st_mode))
	{
		if(unlink(path.c_str()) != 0)
			return filesystemToolError(options, "delete", path);
	}
	else
	{
		errno = EINVAL;
		return filesystemToolError(options, "delete unsupported node", path);
	}

	return true;
}
