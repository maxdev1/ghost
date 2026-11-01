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

#include "source.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static bool installSourceError(const char** error, const char* text)
{
	*error = text;
	return false;
}

static bool installSourceWalk(const std::string& root, const std::string& path, size_t parent,
							  std::vector<g_install_entry>* entries, unsigned depth, const char** error)
{
	if(depth > 64)
		return installSourceError(error, "source directory nesting is too deep");
	auto source = root + (path.empty() ? "" : "/") + path;
	auto directory = opendir(source.c_str());
	if(!directory)
		return installSourceError(error, "could not read a source directory");
	bool success = true;
	for(;;)
	{
		errno = 0;
		auto entry = readdir(directory);
		if(!entry)
		{
			if(errno)
				success = installSourceError(error, "source directory iteration failed");
			break;
		}
		if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		auto relative = path.empty() ? entry->d_name : path + "/" + entry->d_name;
		if(relative == "system/install" || relative == "boot/ramdisk" || relative == "applications/install.bin" ||
		   relative == "user/desktop/10-install.launcher" || relative == "user/start-menu/50-install.launcher")
			continue;
		struct stat info;
		if(stat((root + "/" + relative).c_str(), &info) || (!S_ISREG(info.st_mode) && !S_ISDIR(info.st_mode)) ||
		   info.st_size < 0 || (S_ISREG(info.st_mode) && uint64_t(info.st_size) > UINT32_MAX))
		{
			success = installSourceError(error, "source file is unavailable or has an unsupported type or size");
			break;
		}
		auto index = entries->size();
		entries->push_back(
				{relative, S_ISDIR(info.st_mode) ? 0 : uint64_t(info.st_size), parent, bool(S_ISDIR(info.st_mode))});
		if(S_ISDIR(info.st_mode) && !installSourceWalk(root, relative, index, entries, depth + 1, error))
		{
			success = false;
			break;
		}
	}
	if(closedir(directory))
		success = installSourceError(error, "could not close a source directory");
	return success;
}

bool installSourceCollect(std::vector<g_install_entry>* entries, const char** error, const std::string& root)
{
	entries->clear();
	entries->push_back({"", 0, 0, true});

	if(!installSourceWalk(root, "", 0, entries, 0, error))
		return false;

	for(auto& entry: *entries)
		if(entry.path == "boot/limine/limine.conf")
		{
			for(const auto& other: *entries)
				if(other.path == "limine.conf")
					return installSourceError(error, "conflicting root Limine configuration in the live source");
			entry.sourcePath = entry.path;
			entry.path = "limine.conf";
			entry.parent = 0;
			break;
		}
	return true;
}

static const std::string& installSourcePath(const g_install_entry& entry)
{
	return entry.sourcePath.empty() ? entry.path : entry.sourcePath;
}

g_install_source_tree::~g_install_source_tree()
{
	if(file >= 0)
		close(file);
}

bool installSourcePrepare(g_install_source_tree* tree, const char** error)
{
	tree->plan.bytes = 0;
	bool found = false;
	for(const auto& entry: tree->plan.entries)
	{
		if(entry.size > UINT64_MAX - tree->plan.bytes)
			return installSourceError(error, "source size overflow");
		tree->plan.bytes += entry.size;

		if(installSourcePath(entry) != "boot/limine/limine.conf")
			continue;

		if(entry.directory || entry.size > 1048576)
			return installSourceError(error, "invalid Limine configuration");

		auto file = open((tree->root + "/" + installSourcePath(entry)).c_str(), O_RDONLY);
		if(file < 0)
			return installSourceError(error, "cannot read Limine configuration");

		std::vector<uint8_t> data(entry.size);
		size_t offset = 0;
		while(offset < data.size())
		{
			auto count = read(file, data.data() + offset, data.size() - offset);
			if(count <= 0)
				break;
			offset += count;
		}

		bool valid = offset == data.size();
		if(close(file))
			valid = false;

		if(!valid)
			return installSourceError(error, "cannot read Limine configuration");

		for(size_t i = 0; i + 17 <= data.size(); ++i)
			if(!memcmp(data.data() + i, "root=fat:00000000", 17))
			{
				if(found)
					return installSourceError(error, "duplicate disk identity in Limine configuration");

				tree->patchOffset = i + 9;
				if(i + 26 <= data.size() && data[i + 17] == ':')
					tree->partitionPatchOffset = i + 18;
				found = true;
			}
	}
	return found || installSourceError(error, "disk identity is absent from Limine configuration");
}

bool installSourceRead(void* context, size_t index, uint64_t offset, uint8_t* bytes, size_t count)
{
	auto tree = static_cast<g_install_source_tree*>(context);
	const auto& entry = tree->plan.entries[index];

	if(offset > entry.size || count > entry.size - offset)
		return false;

	if(tree->current != index)
	{
		if(tree->file >= 0 && close(tree->file))
		{
			tree->file = -1;
			tree->current = SIZE_MAX;
			return false;
		}
		tree->file = open((tree->root + "/" + installSourcePath(entry)).c_str(), O_RDONLY);
		tree->current = index;
	}

	struct stat info;
	if(tree->file < 0 || fstat(tree->file, &info) || uint64_t(info.st_size) != entry.size ||
	   lseek(tree->file, offset, SEEK_SET) != int64_t(offset))
		return false;

	size_t done = 0;
	while(done < count)
	{
		auto result = read(tree->file, bytes + done, count - done);
		if(result < 0 && errno == EINTR)
			continue;
		if(result <= 0)
			return false;
		done += result;
	}

	if(installSourcePath(entry) == "boot/limine/limine.conf")
	{
		const char* hex = "0123456789abcdef";
		for(unsigned i = 0; i < 8; ++i)
		{
			if(tree->patchOffset + i >= offset && tree->patchOffset + i - offset < count)
				bytes[tree->patchOffset + i - offset] = hex[(tree->plan.serial >> (28 - i * 4)) & 15];
			if(tree->partitionPatchOffset != UINT64_MAX && tree->partitionPatchOffset + i >= offset &&
			   tree->partitionPatchOffset + i - offset < count)
				bytes[tree->partitionPatchOffset + i - offset] = hex[(tree->plan.firstSector >> (28 - i * 4)) & 15];
		}
	}
	return true;
}
