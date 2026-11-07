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

#include "../src-common/io.hpp"
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, char** argv)
{
	bool append = false;
	bool options = true;
	const char* path = nullptr;
	for(int i = 1; i < argc; ++i)
	{
		if(options && strcmp(argv[i], "--") == 0)
			options = false;
		else if(options && strcmp(argv[i], "-a") == 0)
			append = true;
		else if(options && (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0))
		{
			printf("Usage: write [-a] FILE\nWrite standard input to FILE. Use -a to append instead of replacing.\n");
			return 0;
		}
		else if((options && argv[i][0] == '-') || path)
		{
			fprintf(stderr, "Usage: write [-a] FILE\n");
			return 1;
		}
		else
			path = argv[i];
	}

	if(!path)
	{
		fprintf(stderr, "Usage: write [-a] FILE\n");
		return 1;
	}

	struct stat inputInfo, outputInfo;
	if(fstat(STDIN_FILENO, &inputInfo) < 0)
	{
		coreReportError("write", "stat", "standard input");
		return 1;
	}

	if(stat(path, &outputInfo) == 0 && inputInfo.st_dev == outputInfo.st_dev &&
	   inputInfo.st_ino == outputInfo.st_ino)
	{
		fprintf(stderr, "write: standard input and '%s' are the same file\n", path);
		return 1;
	}

	int output = open(path, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0666);
	if(output < 0)
	{
		coreReportError("write", "open", path);
		return 1;
	}

	int result = coreTransfer(STDIN_FILENO, output, "write", "standard input", path) ? 1 : 0;
	if(coreClose(output, "write", path))
		result = 1;
	return result;
}
