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

#include "copy.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#ifndef O_LINKFILE
#define O_LINKFILE 0
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif
#include <sys/stat.h>
#include <unistd.h>

static bool installCopyError(const char** error, const char* text)
{
	*error = text;
	return false;
}

static bool installCopyFileError(g_install_source_tree* tree, const char** error,
								 const std::string& path, const char* operation, int code)
{
	tree->copyError = std::string(operation) + ": " + path;
	if(code)
		tree->copyError += std::string(": ") + strerror(code);

	return installCopyError(error, tree->copyError.c_str());
}

/**
 * Amortizes allocation barriers while keeping slow-device requests bounded.
 * Small files cannot promote the window using unrepresentative timings.
 */
size_t installCopyAllocationSize(size_t previous, uint64_t elapsed, bool filled)
{
	if(elapsed > 1000)
		return std::max(size_t(65536), previous / 2);
	if(filled && elapsed < 250)
		return std::min(size_t(1048576), previous * 2);

	return previous;
}

bool installCopyFiles(g_install_source_tree* tree, const std::string& destination, bool verify,
					  g_install_progress progress, const char** error, bool overwrite)
{
	uint64_t done = 0;
	size_t allocationBytes = 262144;

	if(progress)
		progress(verify ? "Verifying" : "Copying", 0, tree->plan.bytes);

	uint8_t bytes[G_INSTALL_BUFFER_SIZE], actual[G_INSTALL_BUFFER_SIZE];
	for(size_t i = 1; i < tree->plan.entries.size(); ++i)
	{
		const auto& entry = tree->plan.entries[i];
		auto path = destination + "/" + entry.path;
		if(entry.directory)
		{
			errno = 0;
			struct stat info;
			bool valid = verify ? !lstat(path.c_str(), &info) && S_ISDIR(info.st_mode) : !mkdir(path.c_str(), 0777);
			if(!valid && !verify && overwrite && errno == EEXIST)
				valid = !lstat(path.c_str(), &info) && S_ISDIR(info.st_mode);
			if(!valid)
				return installCopyFileError(tree, error, path,
											verify ? "cannot verify directory" : "cannot create directory", errno);
			continue;
		}

		auto flags = verify ? O_RDONLY : O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : O_EXCL);
		auto file = open(path.c_str(), flags | O_LINKFILE | O_NOFOLLOW, 0666);
		if(file < 0)
			return installCopyFileError(tree, error, path, "cannot open target file", errno);

		const char* operation = nullptr;
		int failure = 0;
		auto fail = [&](const char* action, int code)
		{
			if(!operation)
			{
				operation = action;
				failure = code;
			}
		};

		struct stat info;
		if(verify)
		{
			if(fstat(file, &info))
				fail("cannot inspect target file", errno);
			else if(uint64_t(info.st_size) != entry.size)
				fail("file size verification failed", 0);
		}

		uint64_t allocated = 0;
		for(uint64_t offset = 0; !operation && offset < entry.size;)
		{
			auto count = size_t(std::min(uint64_t(sizeof(bytes)), entry.size - offset));
			if(!verify && offset + count > allocated)
			{
				allocated = std::min(entry.size, offset + allocationBytes);
				auto started = tree->clock ? tree->clock() : 0;
				if(ftruncate(file, allocated))
				{
					fail("cannot allocate target file", errno);
					break;
				}

				if(tree->clock)
				{
					auto finished = tree->clock();
					if(finished >= started)
						allocationBytes = installCopyAllocationSize(allocationBytes, finished - started,
																	allocated - offset == allocationBytes);
				}
			}

			errno = 0;
			if(!installSourceRead(tree, i, offset, bytes, count))
			{
				fail("cannot read source for target file", errno ? errno : EIO);
				break;
			}

			size_t transferred = 0;
			while(transferred < count)
			{
				errno = 0;
				auto result = verify ? read(file, actual + transferred, count - transferred)
									 : write(file, bytes + transferred, count - transferred);
				if(result < 0 && errno == EINTR)
					continue;
				if(result <= 0)
				{
					fail(verify ? "cannot read target file" : "cannot write target file", result < 0 ? errno : EIO);
					break;
				}

				transferred += result;
			}

			if(verify && !operation && memcmp(bytes, actual, count))
				fail("file contents verification failed", 0);
			offset += count;
			if(progress)
				progress(verify ? "Verifying" : "Copying", done + offset, tree->plan.bytes);
		}

		if(!operation && !verify && fsync(file))
			fail("cannot flush target file", errno);
		if(close(file))
			fail("cannot close target file", errno);
		if(operation)
			return installCopyFileError(tree, error, path, operation, failure);
		done += entry.size;
	}

	return true;
}

/**
 * Checks path conflicts and conservative additional allocation before copying.
 * Existing regular files can be replaced; unrelated names are left untouched.
 */
bool installCopyCheck(const g_install_source_tree& tree, const std::string& destination, uint32_t unit,
					  uint64_t freeBytes, const char** error)
{
	if(!unit)
		return installCopyError(error, "invalid filesystem allocation unit");

	uint64_t needed = 0;
	for(size_t i = 1; i < tree.plan.entries.size(); ++i)
	{
		const auto& entry = tree.plan.entries[i];

		struct stat old{};
		bool exists = !lstat((destination + "/" + entry.path).c_str(), &old);
		if(!exists && errno != ENOENT)
			return installCopyError(error, "cannot inspect target path");

		bool compatible = entry.directory ? S_ISDIR(old.st_mode) :
			S_ISREG(old.st_mode) || (O_LINKFILE && S_ISLNK(old.st_mode));
		if(exists && !compatible)
			return installCopyError(error, "a target file conflicts with a source directory or file");

		auto rounded = [&](uint64_t size) { return (size + unit - 1) / unit * unit; };
		uint64_t bytes = entry.directory ? (exists ? 0 : unit) : rounded(entry.size);
		if(!entry.directory && exists)
			bytes -= std::min(bytes, rounded(old.st_size));
		if(!exists)
			bytes += uint64_t(2) * unit;
		if(bytes > freeBytes - needed)
			return installCopyError(error, "not enough free space on the selected volume");

		needed += bytes;
	}

	return true;
}
