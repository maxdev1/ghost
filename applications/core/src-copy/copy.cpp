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

#include "../src-common/arguments.hpp"
#include "../src-common/io.hpp"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>

static int copyOpenSource(const char* source, struct stat& inputInfo)
{
	int input = open(source, O_RDONLY);
	if(input < 0)
	{
		coreReportError("copy", "open", source);
		return -1;
	}
	if(fstat(input, &inputInfo) < 0 || !S_ISREG(inputInfo.st_mode))
	{
		fprintf(stderr, "copy: source '%s' must be a readable regular file\n", source);
		coreClose(input, "copy", source);
		return -1;
	}

	return input;
}

static bool copyPrepareDestination(const char* source, const struct stat& inputInfo, std::string& destination)
{
	struct stat outputInfo;
	if(stat(destination.c_str(), &outputInfo) == 0 && S_ISDIR(outputInfo.st_mode))
	{
		const char* basename = strrchr(source, '/');
		destination += '/';
		destination += basename ? basename + 1 : source;
	}
	if(stat(destination.c_str(), &outputInfo) == 0)
	{
		if(inputInfo.st_dev == outputInfo.st_dev && inputInfo.st_ino == outputInfo.st_ino)
		{
			fprintf(stderr, "copy: '%s' and '%s' are the same file\n", source, destination.c_str());
			return false;
		}
		if(!S_ISREG(outputInfo.st_mode))
		{
			fprintf(stderr, "copy: destination '%s' must be a regular file\n", destination.c_str());
			return false;
		}
	}

	return true;
}

int main(int argc, char** argv)
{
	int first = 1;
	if(argc > 1 && coreArgumentIsHelp(argv[1]))
	{
		printf("Usage: copy SOURCE DESTINATION\nCopy a file to a file or an existing directory, replacing an existing "
			   "file.\n");
		return 0;
	}
	if(argc > 1 && strcmp(argv[1], "--") == 0)
		++first;
	if(argc - first != 2 || (first == 1 && argv[first][0] == '-'))
	{
		fprintf(stderr, "Usage: copy SOURCE DESTINATION\n");
		return 1;
	}

	const char* source = argv[first];
	struct stat inputInfo;
	int input = copyOpenSource(source, inputInfo);
	if(input < 0)
		return 1;

	std::string destination = argv[first + 1];
	if(!copyPrepareDestination(source, inputInfo, destination))
	{
		coreClose(input, "copy", source);
		return 1;
	}

	int output = open(destination.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(output < 0)
	{
		coreReportError("copy", "open", destination.c_str());
		coreClose(input, "copy", source);
		return 1;
	}
	int result = coreTransfer(input, output, "copy", source, destination.c_str()) ? 1 : 0;
	if(coreClose(output, "copy", destination.c_str()))
		result = 1;
	if(coreClose(input, "copy", source))
		result = 1;
	return result;
}
